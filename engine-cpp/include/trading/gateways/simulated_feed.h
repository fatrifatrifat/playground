#pragma once

#include <trading/interfaces/i_market_data_feed.h>

#include <functional>
#include <map>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace quarcc {

// A thread computing simulated prices starting at 100.0 that move with a random
// walk
// Emits ticks and bars based on periods for subscribed symbols
class SimulatedFeed final : public IMarketDataFeed {
public:
  void start() override;
  void stop() override;

  void subscribe(const Symbol &symbol,
                 std::optional<BarPeriod> period) override;
  void unsubscribe(const Symbol &symbol,
                   std::optional<BarPeriod> period) override;

  void set_bar_handler(std::function<void(const Bar &)> handler) override;
  void set_tick_handler(std::function<void(const Tick &)> handler) override;

private:
  void emit_loop(std::stop_token st);

  struct BarAccumulator {
    Bar bar;
    int64_t bar_start_ns{0}; // 0 = no active bar
    double vwap_notional{0.0};
    double vwap_volume{0.0};
    std::chrono::nanoseconds period_ns{0};
  };

  static int64_t align_to_period_start(int64_t ts_ns,
                                       std::chrono::nanoseconds period_ns);
  static void start_bar(BarAccumulator &acc, const Symbol &symbol,
                        const BarPeriod &period, const Tick &tick);
  static void update_bar(BarAccumulator &acc, const Tick &tick);

  std::function<void(const Tick &)> tick_handler_;
  std::function<void(const Bar &)> bar_handler_;

  // Unique symbols that need simulated prices for either ticks or bars
  std::vector<Symbol> symbols_;

  // Tick subscription counts by symbol. Bar only subscriptions are not included
  // here, so they do not receive tick events
  std::map<Symbol, int> tick_subscriptions_;

  // Bar subscription counts by (symbol, period)
  std::map<std::pair<Symbol, BarPeriod>, int> bar_subscriptions_;

  // Per (symbol, period) bar accumulator
  std::map<std::pair<Symbol, BarPeriod>, BarAccumulator> accumulators_;
  std::jthread thread_;
  std::mutex data_mu_;
};

} // namespace quarcc
