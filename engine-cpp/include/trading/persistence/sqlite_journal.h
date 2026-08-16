#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <sqlite3.h>
#include <thread>
#include <trading/interfaces/i_journal.h>

namespace quarcc {

class SQLiteJournal : public IJournal {
public:
  explicit SQLiteJournal(const std::string &db_path);
  ~SQLiteJournal() override;

  SQLiteJournal(const SQLiteJournal &) = delete;
  SQLiteJournal &operator=(const SQLiteJournal &) = delete;

  void log(Event event, const std::string &data,
           const std::string &correlation_id = "") override;

  Result<std::vector<LogEntry>>
  get_history(Timestamp from, Timestamp to,
              std::optional<Event> event_filter = std::nullopt) override;

  std::vector<LogEntry> get_order_history(const std::string &order_id) override;

  void flush() override;

  static constexpr auto MAX_QUEUE_DEPTH = 4096uz;
  static constexpr auto BATCH_SIZE = 64uz;
  static constexpr auto BATCH_INTERVAL_MS = 50;

private:
  struct PendingEntry {
    std::string timestamp;
    Event event;
    std::string data;
    std::string correlation_id;
  };

  void create_schema();
  void writer_loop(std::stop_token st);
  void commit_batch(const std::vector<PendingEntry> &batch);

  sqlite3 *db_ = nullptr;
  mutable std::mutex db_mu_; // Guards all SQLite operations

  std::deque<PendingEntry> write_queue_;
  std::mutex queue_mu_; // Guards the pending entries queue
  std::condition_variable_any queue_cv_;
  std::uint64_t pushed_{};
  std::uint64_t committed_{};

  // Must be declared last
  std::jthread writer_thread_;
};

} // namespace quarcc
