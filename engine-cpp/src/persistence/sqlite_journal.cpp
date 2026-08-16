#include <spdlog/spdlog.h>
#include <trading/persistence/sqlite_journal.h>

#include <iostream>
#include <stdexcept>
#include <trading/utils/helpers.h>

namespace quarcc {

SQLiteJournal::SQLiteJournal(const std::string &db_path) {
  std::string full_path =
      (db_path == ":memory:") ? db_path : std::format("{}_journal.db", db_path);
  int rc = sqlite3_open(full_path.c_str(), &db_);
  if (rc != SQLITE_OK) [[unlikely]] {
    std::string error = sqlite3_errmsg(db_);
    throw std::runtime_error("Failed to open journal database: " + error);
  }

  sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
  sqlite3_exec(db_, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);
  sqlite3_exec(db_, "PRAGMA cache_size=-8000;", nullptr, nullptr,
               nullptr); // 8MB cache

  create_schema();

  writer_thread_ =
      std::jthread([this](std::stop_token st) { writer_loop(st); });
}

// Shutdown order:
//  1. flush() -> waits until all pushed pending entries are committed
//  2. request_stop() to signal the writer thread to exit
//  3. notify_all() to wake the writer thread
//  4. join() wait for the writer thread to exit
//  5. WAL checkpoint + sqlite3_close
//
SQLiteJournal::~SQLiteJournal() {
  flush();
  writer_thread_.request_stop();
  queue_cv_.notify_all();
  if (writer_thread_.joinable())
    writer_thread_.join();

  if (db_) {
    std::lock_guard lk{db_mu_};
    sqlite3_wal_checkpoint(db_, nullptr);
    sqlite3_close(db_);
    db_ = nullptr;
  }
}

void SQLiteJournal::create_schema() {
  const char *sql = R"(
    CREATE TABLE IF NOT EXISTS journal (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      timestamp TEXT NOT NULL,
      event_type TEXT NOT NULL,
      data TEXT NOT NULL,
      correlation_id TEXT,
      UNIQUE(timestamp, correlation_id, event_type)
    );
    
    CREATE INDEX IF NOT EXISTS idx_timestamp ON journal(timestamp);
    CREATE INDEX IF NOT EXISTS idx_event_type ON journal(event_type);
    CREATE INDEX IF NOT EXISTS idx_correlation_id ON journal(correlation_id);
  )";

  char *err_msg = nullptr;
  int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &err_msg);

  if (rc != SQLITE_OK) [[unlikely]] {
    std::string error = err_msg;
    sqlite3_free(err_msg);
    throw std::runtime_error("Failed to create journal schema: " + error);
  }
}

void SQLiteJournal::log(Event event, const std::string &data,
                        const std::string &correlation_id) {
  std::string timestamp_str{"UNKNOWN"};
  try {
    timestamp_str = LogEntry::timestamp_to_string(LogEntry::now());
  } catch (const std::exception &err) {
    spdlog::error("[journal] timestamp conversion failed: {}", err.what());
  }

  std::lock_guard lk{queue_mu_};
  if (write_queue_.size() >= MAX_QUEUE_DEPTH) {
    spdlog::warn("[journal] queue full ({} entries); dropping log entry",
                 write_queue_.size());
    return;
  }

  write_queue_.push_back(PendingEntry{.timestamp = std::move(timestamp_str),
                                      .event = event,
                                      .data = data,
                                      .correlation_id = correlation_id});
  ++pushed_;
  queue_cv_.notify_one();
}

Result<std::vector<LogEntry>>
SQLiteJournal::get_history(Timestamp from, Timestamp to,
                           std::optional<Event> event_filter) {
  std::lock_guard lock(db_mu_);
  std::vector<LogEntry> entries;

  std::string sql = R"(
    SELECT id, timestamp, event_type, data, correlation_id 
    FROM journal 
    WHERE timestamp BETWEEN ? AND ?
  )";

  if (event_filter) {
    sql += " AND event_type = ?";
  }

  sql += " ORDER BY id ASC";

  sqlite3_stmt *stmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);

  if (rc != SQLITE_OK) [[unlikely]] {
    spdlog::error("Failed to prepare query: {}", sqlite3_errmsg(db_));
    return entries;
  }

  std::string from_str;
  std::string to_str;

  try {
    from_str = LogEntry::timestamp_to_string(from);
  } catch (const std::exception &err) {
    return std::unexpected{
        Error{std::string{err.what()} +
                  " (Occured when converting timestamp to string : from)",
              ErrorType::Error}};
  }

  try {
    to_str = LogEntry::timestamp_to_string(to);
  } catch (const std::exception &err) {
    return std::unexpected{
        Error{std::string{err.what()} +
                  " (Occured when converting timestamp to string : to)",
              ErrorType::Error}};
  }

  sqlite3_bind_text(stmt, 1, from_str.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, to_str.c_str(), -1, SQLITE_TRANSIENT);

  if (event_filter) {
    sqlite3_bind_text(stmt, 3, event_to_string(*event_filter), -1,
                      SQLITE_STATIC);
  }

  // Fetch results
  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    LogEntry entry;
    entry.id = sqlite3_column_int64(stmt, 0);

    const char *timestamp_str =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    entry.timestamp = LogEntry::string_to_timestamp(timestamp_str);

    entry.event_type = event_from_string(
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)));

    const char *data_str =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
    entry.data = data_str ? data_str : "";

    const char *corr_id =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
    entry.correlation_id = corr_id ? corr_id : "";

    entries.push_back(std::move(entry));
  }

  sqlite3_finalize(stmt);
  return entries;
}

std::vector<LogEntry>
SQLiteJournal::get_order_history(const std::string &order_id) {
  std::lock_guard lock(db_mu_);
  std::vector<LogEntry> entries;

  const char *sql = R"(
    SELECT id, timestamp, event_type, data, correlation_id 
    FROM journal 
    WHERE correlation_id = ?
    ORDER BY id ASC
  )";

  sqlite3_stmt *stmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

  if (rc != SQLITE_OK) [[unlikely]] {
    spdlog::error("Failed to prepare query: {}", sqlite3_errmsg(db_));
    return entries;
  }

  sqlite3_bind_text(stmt, 1, order_id.c_str(), -1, SQLITE_TRANSIENT);

  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    LogEntry entry;
    entry.id = sqlite3_column_int64(stmt, 0);

    const char *timestamp_str =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    entry.timestamp = LogEntry::string_to_timestamp(timestamp_str);

    entry.event_type = event_from_string(
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)));

    const char *data_str =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
    entry.data = data_str ? data_str : "";

    const char *corr_id =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
    entry.correlation_id = corr_id ? corr_id : "";

    entries.push_back(std::move(entry));
  }

  sqlite3_finalize(stmt);
  return entries;
}

void SQLiteJournal::flush() {
  {
    std::unique_lock lk{queue_mu_};
    queue_cv_.wait(lk, [&] { return committed_ >= pushed_; });
  }

  std::lock_guard lock(db_mu_);
  if (db_)
    sqlite3_wal_checkpoint(db_, nullptr);
}

void SQLiteJournal::writer_loop(std::stop_token st) {
  while (true) {
    std::vector<PendingEntry> batch;

    {
      std::unique_lock lk{queue_mu_};
      queue_cv_.wait_for(lk, st, std::chrono::milliseconds{BATCH_INTERVAL_MS},
                         [&] { return !write_queue_.empty(); });

      if (write_queue_.empty()) {
        if (st.stop_requested()) {
          break;
        }
        continue;
      }

      batch.reserve(std::min(write_queue_.size(), BATCH_SIZE));
      while (!write_queue_.empty() && batch.size() < BATCH_SIZE) {
        batch.push_back(std::move(write_queue_.front()));
        write_queue_.pop_front();
      }
    }

    commit_batch(batch);

    {
      std::lock_guard lk{queue_mu_};
      committed_ += batch.size();
      queue_cv_.notify_all(); // wakes flush() if it's waiting
    }
  }
}

void SQLiteJournal::commit_batch(const std::vector<PendingEntry> &batch) {
  if (batch.empty())
    return;

  std::lock_guard lk{db_mu_};
  if (!db_)
    return;

  sqlite3_exec(db_, "BEGIN", nullptr, nullptr, nullptr);

  const char *sql = R"(
    INSERT OR IGNORE INTO journal (timestamp, event_type, data, correlation_id)
    VALUES (?, ?, ?, ?)
  )";

  sqlite3_stmt *stmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

  if (rc != SQLITE_OK) [[unlikely]] {
    spdlog::error("[journal] prepare failed: {}", sqlite3_errmsg(db_));
    sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr);
    return;
  }

  for (const auto &e : batch) {
    sqlite3_reset(stmt);

    sqlite3_bind_text(stmt, 1, e.timestamp.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, event_to_string(e.event), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, e.data.c_str(), -1, SQLITE_TRANSIENT);

    if (!e.correlation_id.empty()) {
      sqlite3_bind_text(stmt, 4, e.correlation_id.c_str(), -1,
                        SQLITE_TRANSIENT);
    } else {
      sqlite3_bind_null(stmt, 4);
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) [[unlikely]]
      spdlog::error("[journal] insert failed: {}", sqlite3_errmsg(db_));
  }

  sqlite3_finalize(stmt);
  sqlite3_exec(db_, "COMMIT", nullptr, nullptr, nullptr);
}

} // namespace quarcc
