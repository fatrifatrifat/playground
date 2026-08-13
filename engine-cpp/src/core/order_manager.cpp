#include <spdlog/spdlog.h>
#include <trading/core/order_manager.h>
#include <trading/core/position_keeper.h>
#include <trading/utils/event_queue.h>

#include <filesystem>

namespace quarcc {

namespace {

template <typename T>
concept OrderSignal = requires(const T &s) {
  s.symbol();
  s.side();
  s.target_quantity();
  s.strategy_id();
};

template <OrderSignal T>
v1::Order create_order_from_signal(const T &signal,
                                   std::string_view account_id) {
  v1::Order order;
  order.set_symbol(signal.symbol());
  order.set_side(signal.side());
  order.set_quantity(signal.target_quantity());
  order.set_type(v1::OrderType::MARKET);
  order.set_account_id(std::string(account_id));
  order.set_created_at(get_current_time());
  order.set_time_in_force(v1::TimeInForce::DAY);
  order.set_strategy_id(signal.strategy_id());
  return order;
}

} // namespace

std::unique_ptr<OrderManager> OrderManager::create_order_manager(
    std::string account_id, std::unique_ptr<PositionKeeper> pk,
    std::unique_ptr<IExecutionGateway> gw, std::unique_ptr<IJournal> lj,
    std::unique_ptr<IOrderStore> os, std::unique_ptr<RiskManager> rm) {
  return std::unique_ptr<OrderManager>(
      new OrderManager(std::move(account_id), std::move(pk), std::move(gw),
                       std::move(lj), std::move(os), std::move(rm)));
}

OrderManager::OrderManager(std::string account_id,
                           std::unique_ptr<PositionKeeper> pk,
                           std::unique_ptr<IExecutionGateway> gw,
                           std::unique_ptr<IJournal> lj,
                           std::unique_ptr<IOrderStore> os,
                           std::unique_ptr<RiskManager> rm)
    : account_id_(std::move(account_id)), position_keeper_(std::move(pk)),
      gateway_(std::move(gw)), journal_(std::move(lj)),
      order_store_(std::move(os)), risk_manager_(std::move(rm)),
      id_generator_(std::make_unique<OrderIdGenerator>()),
      id_mapper_(std::make_unique<OrderIdMapper>()) {
  gateway_->set_fill_handler(
      [this](const v1::ExecutionReport &r) { enqueue(r); });

  dispatch_thread_ =
      std::jthread([this](std::stop_token st) { run_dispatch_loop(st); });

  gateway_->start();
}

// Shutdown order:
//  1. gateway_->stop() drains any remaining fills into the queue
//  2. dispatch_thread_'s destructor requests stop and joins
//  3. The dispatch loop drains the queue (processes all remaining events,
//     including any CancelAllCommand already in flight) before exiting
//  4. Members are destroyed in reverse order they were declared
OrderManager::~OrderManager() { gateway_->stop(); }

void OrderManager::enqueue(OMEvent event) {
  // Can drop market data if there's too much to process, it's okay to drop them
  // This doesn't apply to v1::ExecutionReport because it CANNOT be dropped
  if (std::holds_alternative<Bar>(event) ||
      std::holds_alternative<Tick>(event)) {
    if (!queue_.try_push(std::move(event), MAX_MARKETDATA_QUEUE_DEPTH))
      spdlog::warn(
          "[OrderManager] Market data dropped - queue full ({} total drops)",
          queue_.dropped_count());
    return;
  }
  queue_.push(std::move(event));
}

void OrderManager::wait_idle() { queue_.wait_idle(); }

void OrderManager::run_dispatch_loop(std::stop_token st) {
  OMEvent event;
  while (queue_.pop(event, st)) {
    std::visit(
        overloaded{
            [this](const v1::ExecutionReport &r) { handle_fill(r); },
            [this](const Bar &b) { handle_bar(b); },
            [this](const Tick &t) { handle_tick(t); },
            // Set as non const to mutate std::promise<T> inside
            [this](SubmitCommand &cmd) { handle_submit_command(cmd); },
            [this](CancelCommand &cmd) { handle_cancel_command(cmd); },
            [this](ReplaceCommand &cmd) { handle_replace_command(cmd); },
            [this](CancelAllCommand &cmd) { handle_cancel_all_command(cmd); },
        },
        event);
    queue_.mark_processed();
  }
}

Result<LocalOrderId>
OrderManager::process_signal(const v1::StrategySignal &signal) {
  std::promise<Result<LocalOrderId>> promise;
  auto future = promise.get_future();
  enqueue(SubmitCommand{signal, std::move(promise)});
  return future.get();
}

Result<std::monostate>
OrderManager::process_signal(const v1::CancelSignal &signal) {
  std::promise<Result<std::monostate>> promise;
  auto future = promise.get_future();
  enqueue(CancelCommand{signal, std::move(promise)});
  return future.get();
}

Result<LocalOrderId>
OrderManager::process_signal(const v1::ReplaceSignal &signal) {
  std::promise<Result<LocalOrderId>> promise;
  auto future = promise.get_future();
  enqueue(ReplaceCommand{signal, std::move(promise)});
  return future.get();
}

void OrderManager::cancel_all(const std::string &reason,
                              const std::string &initiated_by) {
  std::promise<void> done;
  auto future = done.get_future();
  enqueue(CancelAllCommand{reason, initiated_by, std::move(done)});
  future.get();
}

// Steps:
//   1. Generate local_id, build the order
//   2. Persist PENDING_SUBMISSION to order_store_
//   3. Risk manager check
//   4. Call gateway_->submit_order() (synchronous adapter RPC)
//   5. Persist SUBMITTED status
//   6. Register local to broker mapping (THE KEY ORDERING GUARANTEE)
//   7. Increment open_order_count_
//   8. Persist broker_id to order_store_
//   9. Fulfill the promise with local_id
//
void OrderManager::handle_submit_command(SubmitCommand &cmd) {
  std::string local_id = id_generator_->generate();
  v1::Order order = create_order_from_signal(cmd.signal, account_id_);
  order.set_id(local_id);
  journal_->log(Event::ORDER_CREATED, order.DebugString(), order.id());

  StoredOrder stored;
  stored.order = order;
  stored.local_id = local_id;
  stored.status = OrderStatus::PENDING_SUBMISSION;

  try {
    stored.created_at = LogEntry::timestamp_to_string(LogEntry::now());
  } catch (const std::exception &err) {
    cmd.result.set_value(std::unexpected{
        Error{std::string{err.what()} +
                  " (timestamp conversion failure in submit command)",
              ErrorType::FailedOrder}});
    return;
  }

  if (auto r = order_store_->store_order(stored); !r) {
    journal_->log(Event::ERROR_OCCURRED, r.error().message_, local_id);
    cmd.result.set_value(std::unexpected(r.error()));
    return;
  }

  const int open_count = open_order_count_;
  const double realized_pnl = position_keeper_->get_total_pnl();
  if (auto r = risk_manager_->check(order.quantity(), open_count, realized_pnl);
      !r) {
    journal_->log(Event::ORDER_REJECTED, r.error().message_, local_id);
    if (auto sr =
            order_store_->update_order_status(local_id, OrderStatus::REJECTED);
        !sr)
      journal_->log(Event::ERROR_OCCURRED, sr.error().message_, local_id);
    cmd.result.set_value(std::unexpected(r.error()));
    return;
  }

  auto result = gateway_->submit_order(order);
  if (!result) {
    journal_->log(Event::ORDER_REJECTED, result.error().message_, local_id);
    if (auto r =
            order_store_->update_order_status(local_id, OrderStatus::REJECTED);
        !r)
      journal_->log(Event::ERROR_OCCURRED, r.error().message_, local_id);
    cmd.result.set_value(std::unexpected(result.error()));
    return;
  }

  const std::string broker_id = result.value();

  if (auto r =
          order_store_->update_order_status(local_id, OrderStatus::SUBMITTED);
      !r) {
    journal_->log(Event::ERROR_OCCURRED, r.error().message_, local_id);
    cmd.result.set_value(std::unexpected(r.error()));
    return;
  }

  id_mapper_->add_mapping(local_id, broker_id);
  open_order_count_++;

  if (auto r = order_store_->update_broker_id(local_id, broker_id); !r)
    journal_->log(Event::ERROR_OCCURRED, r.error().message_, local_id);

  journal_->log(Event::ORDER_SUBMITTED,
                "Local: " + local_id + ", Broker: " + broker_id, local_id);

  cmd.result.set_value(local_id);
}

// TODO: Cancel signal should probably still return something, like the
// ExecutionReport
void OrderManager::handle_cancel_command(CancelCommand &cmd) {
  const std::string &local_id = cmd.signal.order_id();

  auto broker_id = id_mapper_->get_broker_id(local_id);
  if (!broker_id) [[unlikely]] {
    cmd.result.set_value(std::unexpected(Error{
        "Cannot find broker ID for order: " + local_id, ErrorType::Error}));
    return;
  }

  auto result = gateway_->cancel_order(*broker_id);
  if (result) {
    journal_->log(Event::ORDER_CANCELLED, "Cancelled", local_id);
    id_mapper_->remove_mapping(local_id);
    if (auto r =
            order_store_->update_order_status(local_id, OrderStatus::CANCELLED);
        !r) {
      journal_->log(Event::ERROR_OCCURRED, r.error().message_, local_id);
      cmd.result.set_value(std::unexpected(r.error()));
      return;
    }
    open_order_count_--;
  }

  cmd.result.set_value(result);
}

void OrderManager::handle_replace_command(ReplaceCommand &cmd) {
  const std::string old_local_id = cmd.signal.order_id();

  const auto old_broker_id = id_mapper_->get_broker_id(old_local_id);
  if (!old_broker_id) [[unlikely]] {
    cmd.result.set_value(std::unexpected(Error{
        "Cannot find broker ID for order: " + old_local_id, ErrorType::Error}));
    return;
  }

  std::string new_local_id = id_generator_->generate();
  v1::Order new_order = create_order_from_signal(cmd.signal, account_id_);
  new_order.set_id(new_local_id);

  journal_->log(Event::ORDER_REPLACED,
                "Replacing " + old_local_id + " with " + new_local_id,
                new_local_id);

  auto result = gateway_->replace_order(*old_broker_id, new_order);
  if (!result) {
    journal_->log(Event::ORDER_REJECTED, result.error().message_, new_local_id);
    cmd.result.set_value(std::unexpected(result.error()));
    return;
  }

  const std::string new_broker_id = result.value();

  // Remove old mapping BEFORE adding the new one so a stray fill on the old
  // broker_id after the replace is correctly rejected in handle_fill().
  id_mapper_->remove_mapping(old_local_id);

  if (auto r = order_store_->update_order_status(old_local_id,
                                                 OrderStatus::REPLACED);
      !r) {
    journal_->log(Event::ERROR_OCCURRED, r.error().message_, old_local_id);
    cmd.result.set_value(std::unexpected(r.error()));
    return;
  }

  StoredOrder stored;
  stored.order = new_order;
  stored.local_id = new_local_id;
  stored.broker_id = new_broker_id;
  stored.status = OrderStatus::SUBMITTED;

  try {
    stored.created_at = LogEntry::timestamp_to_string(LogEntry::now());
  } catch (const std::exception &err) {
    cmd.result.set_value(std::unexpected{
        Error{std::string{err.what()} +
                  " (timestamp conversion failure in replace command)",
              ErrorType::FailedOrder}});
    return;
  }

  if (auto r = order_store_->store_order(stored); !r) {
    journal_->log(Event::ERROR_OCCURRED, r.error().message_, new_local_id);
    cmd.result.set_value(std::unexpected(r.error()));
    return;
  }

  // Phase 1 fill-race fix for replace: new mapping registered here, before
  // any fill on new_broker_id can be dequeued by the dispatch loop.
  id_mapper_->add_mapping(new_local_id, new_broker_id);

  journal_->log(Event::ORDER_SUBMITTED,
                std::format("Old: {} -> New: {} (Broker: {})", old_local_id,
                            new_local_id, new_broker_id),
                new_local_id);

  cmd.result.set_value(new_local_id);
}

void OrderManager::handle_cancel_all_command(CancelAllCommand &cmd) {
  const std::string ks_data = "Kill switch activated by: " + cmd.initiated_by +
                              ". Reason: " + cmd.reason;
  journal_->log(Event::KILL_SWITCH_ACTIVATED, ks_data);

  const auto open_orders = order_store_->get_open_orders();
  for (const auto &stored : open_orders) {
    if (!stored.broker_id)
      continue;

    if (auto r = gateway_->cancel_order(*stored.broker_id); r) {
      if (auto r = order_store_->update_order_status(stored.local_id,
                                                     OrderStatus::CANCELLED);
          !r) [[unlikely]] {
        journal_->log(Event::ERROR_OCCURRED, r.error().message_,
                      stored.local_id);
        continue;
      }
      id_mapper_->remove_mapping(stored.local_id);
      journal_->log(Event::ORDER_CANCELLED, "Cancelled by kill switch",
                    stored.local_id);
    } else [[unlikely]] {
      journal_->log(Event::ERROR_OCCURRED,
                    "Failed to cancel during kill switch: " +
                        r.error().message_,
                    stored.local_id);
    }
  }

  cmd.done.set_value();
}

// Steps:
//  1. Resolve broker to local ID
//  2. Handle REJECTED/CANCELLED status early
//  3. Fetch stored order to compare filled vs original quantity
//  4. Persist fill details + determine FILLED or PARTIALLY_FILLED
//  5. Update in memory position
//  6. Journal the event
//  7. Remove fully filled orders from the mapper
//  8. Notify fill sink (StreamFills gRPC stream)
//
void OrderManager::handle_fill(const v1::ExecutionReport &fill) {
  const std::string &broker_id = fill.broker_order_id();

  // 1. Resolve broker to local ID.
  auto local_id_opt = id_mapper_->get_local_id(broker_id);
  if (!local_id_opt) {
    spdlog::error("[handle_fill] Unknown broker_id '{}' not submitted through "
                  "this engine (external order or duplicate stream event?)",
                  broker_id);
    return;
  }
  const std::string &local_id = *local_id_opt;

  // 2. Early exit for terminal statuses that carry no fill data
  const auto proto_status = fill.status();
  if (proto_status == v1::OrderStatus::REJECTED ||
      proto_status == v1::OrderStatus::CANCELLED) {
    const OrderStatus new_status = (proto_status == v1::OrderStatus::REJECTED)
                                       ? OrderStatus::REJECTED
                                       : OrderStatus::CANCELLED;

    if (auto r = order_store_->update_order_status(local_id, new_status); !r)
      journal_->log(Event::ERROR_OCCURRED, r.error().message_, local_id);

    id_mapper_->remove_mapping(local_id);
    {
      std::lock_guard lk{fill_sink_mu_};
      if (fill_sink_) [[likely]]
        fill_sink_(fill);
    }
    return;
  }

  // 3. Fetch stored order to compare filled vs original quantity
  auto stored = order_store_->get_order(local_id);
  if (!stored) {
    journal_->log(Event::ERROR_OCCURRED,
                  "Cannot find stored order for local_id: " + local_id,
                  local_id);
    return;
  }

  const double filled_qty = fill.filled_quantity();
  const double original_qty = stored->order.quantity();

  // 4. Determine status and persist fill details + status
  const bool fully_filled =
      (stored->filled_quantity + filled_qty >= original_qty);
  const OrderStatus new_status =
      fully_filled ? OrderStatus::FILLED : OrderStatus::PARTIALLY_FILLED;

  if (auto r = order_store_->apply_fill(local_id, filled_qty,
                                        fill.avg_fill_price(), new_status);
      !r)
    journal_->log(Event::ERROR_OCCURRED, r.error().message_, local_id);

  // 5. Update in memory position
  position_keeper_->on_fill(fill.symbol(), filled_qty, fill.avg_fill_price(),
                            fill.side());

  // 6. Journal the event
  journal_->log(fully_filled ? Event::ORDER_FILLED
                             : Event::ORDER_PARTIALLY_FILLED,
                std::format("Filled: {} / {} @ avg={}", filled_qty,
                            original_qty, fill.avg_fill_price()),
                local_id);

  // 7. Remove fully filled orders from the mapper (terminal state)
  if (fully_filled) {
    id_mapper_->remove_mapping(local_id);
    open_order_count_--;
  }

  // 8. Notify fill sink (StreamFills gRPC stream)
  {
    std::lock_guard lk{fill_sink_mu_};
    if (fill_sink_) [[likely]]
      fill_sink_(fill);
  }
}

void OrderManager::handle_bar(const Bar &bar) {
  std::lock_guard lk{md_sink_mu_};
  if (bar_sink_) [[likely]]
    bar_sink_(bar);
}

void OrderManager::handle_tick(const Tick &tick) {
  std::lock_guard lk{md_sink_mu_};
  if (tick_sink_) [[likely]]
    tick_sink_(tick);
}

void OrderManager::set_market_data_sinks(
    std::function<void(const Tick &)> tick_sink,
    std::function<void(const Bar &)> bar_sink) {
  std::lock_guard lk{md_sink_mu_};
  tick_sink_ = std::move(tick_sink);
  bar_sink_ = std::move(bar_sink);
}

void OrderManager::clear_market_data_sinks() {
  std::lock_guard lk{md_sink_mu_};
  tick_sink_ = nullptr;
  bar_sink_ = nullptr;
}

void OrderManager::set_fill_sink(
    std::function<void(const v1::ExecutionReport &)> sink) {
  std::lock_guard lk{fill_sink_mu_};
  fill_sink_ = std::move(sink);
}

void OrderManager::clear_fill_sink() {
  std::lock_guard lk{fill_sink_mu_};
  fill_sink_ = nullptr;
}

Result<v1::Position>
OrderManager::get_position(const std::string &symbol) const {
  return position_keeper_->get_position(symbol);
}

v1::PositionList OrderManager::get_all_positions() const {
  return position_keeper_->get_all_positions();
}

} // namespace quarcc
