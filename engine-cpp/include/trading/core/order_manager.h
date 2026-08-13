#pragma once

#include "order.pb.h"
#include "strategy_signal.pb.h"

#include <trading/core/position_keeper.h>
#include <trading/core/risk_manager.h>
#include <trading/interfaces/i_execution_gateway.h>
#include <trading/interfaces/i_journal.h>
#include <trading/interfaces/i_market_data_feed.h>
#include <trading/interfaces/i_order_store.h>
#include <trading/utils/event_queue.h>
#include <trading/utils/order_id_generator.h>
#include <trading/utils/order_id_types.h>
#include <trading/utils/result.h>

#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <variant>

namespace quarcc {

struct SubmitCommand {
  v1::StrategySignal signal;
  std::promise<Result<LocalOrderId>> result;
};

struct CancelCommand {
  v1::CancelSignal signal;
  std::promise<Result<std::monostate>> result;
};

struct ReplaceCommand {
  v1::ReplaceSignal signal;
  std::promise<Result<LocalOrderId>> result;
};

struct CancelAllCommand {
  std::string reason;
  std::string initiated_by;
  std::promise<void> done;
};

// All events processed by each order manager
// No lock required, they're all processed sequentially
using OMEvent = std::variant<Bar, Tick, v1::ExecutionReport, SubmitCommand,
                             CancelCommand, ReplaceCommand, CancelAllCommand>;

// Maximum number of market data events (Bar + Tick) buffered per OrderManager
// before new ones are dropped
// Fills and commands are never subject to this can
//
// 2^13 is very random, but seems to give okay perf and doesn't kill my RAM :'(
inline constexpr std::size_t MAX_MARKETDATA_QUEUE_DEPTH = 8192;

class OrderManager {
public:
  OrderManager(const OrderManager &) = delete;
  OrderManager &operator=(const OrderManager &) = delete;
  OrderManager(OrderManager &&) = delete;
  OrderManager &operator=(OrderManager &&) = delete;

  ~OrderManager();

  static std::unique_ptr<OrderManager> create_order_manager(
      std::string account_id, std::unique_ptr<PositionKeeper> pk,
      std::unique_ptr<IExecutionGateway> gw, std::unique_ptr<IJournal> lj,
      std::unique_ptr<IOrderStore> os, std::unique_ptr<RiskManager> rm);

  // May be called by different threads (gRPC, market feed, etc.)
  void enqueue(OMEvent event);

  // Processes all remaining events for a graceful destruction
  // Especially useful for debugging logs after a crash. SO KEEP IT!
  void wait_idle();

  // Sets sink funtions for handling data for bars and ticks given by gRPC to a
  // specific strategy. They're later called by the dispatcher thread whenever a
  // bar/tick event comes inside the queue
  // ALSO: always call clear_*_sinks() before the ServerWriter is
  // destroyed
  void set_market_data_sinks(std::function<void(const Tick &)> tick_sink,
                             std::function<void(const Bar &)> bar_sink);
  void clear_market_data_sinks();
  void set_fill_sink(std::function<void(const v1::ExecutionReport &)> sink);
  void clear_fill_sink();

  // Thin wrappers that enqueue the event for the dispatch thread to handle
  // Waits for the processed return value with std::future/std::promise
  Result<LocalOrderId> process_signal(const v1::StrategySignal &signal);
  Result<std::monostate> process_signal(const v1::CancelSignal &signal);
  Result<LocalOrderId> process_signal(const v1::ReplaceSignal &signal);

  void cancel_all(const std::string &reason, const std::string &initiated_by);

  Result<v1::Position> get_position(const std::string &symbol) const;
  v1::PositionList get_all_positions() const;

private:
  OrderManager(std::string account_id, std::unique_ptr<PositionKeeper> pk,
               std::unique_ptr<IExecutionGateway> gw,
               std::unique_ptr<IJournal> lj, std::unique_ptr<IOrderStore> os,
               std::unique_ptr<RiskManager> rm);

  // Big dawg loop that cleans up the event queue by calling the right handler
  // for the queue's front event
  void run_dispatch_loop(std::stop_token st);

  // These contain the logic that previously lived in process_signal() on the
  // gRPC thread. Moving them here makes the dispatch thread the sole writer of
  // order_store_, id_mapper_, and open_order_count_
  void handle_submit_command(SubmitCommand &cmd);
  void handle_cancel_command(CancelCommand &cmd);
  void handle_replace_command(ReplaceCommand &cmd);
  void handle_cancel_all_command(CancelAllCommand &cmd);

  // Original fill/market data handlers
  void handle_fill(const v1::ExecutionReport &fill);
  void handle_bar(const Bar &bar);
  void handle_tick(const Tick &tick);

private:
  std::string account_id_;

  std::unique_ptr<PositionKeeper> position_keeper_;
  std::unique_ptr<IExecutionGateway> gateway_;
  std::unique_ptr<IJournal> journal_;
  std::unique_ptr<IOrderStore> order_store_;
  std::unique_ptr<RiskManager> risk_manager_;
  std::unique_ptr<OrderIdGenerator> id_generator_;
  std::unique_ptr<OrderIdMapper> id_mapper_;

  std::function<void(const Tick &)> tick_sink_;
  std::function<void(const Bar &)> bar_sink_;
  std::mutex md_sink_mu_;

  std::function<void(const v1::ExecutionReport &)> fill_sink_;
  std::mutex fill_sink_mu_;

  // Doesn't need to be atomic anymore since only the dispatch thread mutates it
  int open_order_count_{};

  // IMPORTANT: dispatch_thread_ must be declared last so its destructor joins
  // the thread before any other members (including the queue) are destroyed
  EventQueue<OMEvent> queue_;
  std::jthread dispatch_thread_;
};

} // namespace quarcc
