#include <trading/gateways/simulated_feed.h>
#include <trading/utils/helpers.h>

#include <algorithm>
#include <chrono>
#include <random>
#include <thread>
#include <unordered_map>
#include <vector>

namespace quarcc {

static std::chrono::nanoseconds period_to_ns(BarPeriod p) {
  using namespace std::chrono;

  if (p.size() < 2) {
    throw std::invalid_argument("Invalid BarPeriod");
  }

  std::optional<int> time_amount{};
  {
    const auto time_amt_str = p.substr(0, p.size() - 1);
    time_amount = utils::str_to_int(time_amt_str);
    if (!time_amount.has_value()) {
      throw std::invalid_argument("Invalid BarPeriod");
    }
  }

  if (time_amount.value() <= 0) {
    throw std::invalid_argument("Invalid BarPeriod");
  }

  switch (p.back()) {
  case 's':
    return seconds{time_amount.value()};
  case 'm':
    return minutes{time_amount.value()};
  case 'h':
    return hours{time_amount.value()};
  case 'd':
    return days{time_amount.value()};
  }

  throw std::invalid_argument("Invalid BarPeriod");
}

int64_t
SimulatedFeed::align_to_period_start(int64_t ts_ns,
                                     std::chrono::nanoseconds period_ns) {
  return (ts_ns / period_ns.count()) * period_ns.count();
}

void SimulatedFeed::start_bar(BarAccumulator &acc, const Symbol &symbol,
                              const BarPeriod &period, const Tick &tick) {
  acc.bar_start_ns = align_to_period_start(tick.ts_ns, acc.period_ns);
  acc.vwap_notional = tick.last * tick.last_size;
  acc.vwap_volume = tick.last_size;
  acc.bar = Bar{.symbol = symbol,
                .period = period,
                .open = tick.last,
                .high = tick.last,
                .low = tick.last,
                .close = tick.last,
                .volume = tick.last_size,
                .vwap = tick.last,
                .ts_ns = acc.bar_start_ns,
                .ts_recv_ns = tick.ts_recv_ns};
}

void SimulatedFeed::update_bar(BarAccumulator &acc, const Tick &tick) {
  acc.bar.high = std::max(acc.bar.high, tick.last);
  acc.bar.low = std::min(acc.bar.low, tick.last);
  acc.bar.close = tick.last;
  acc.bar.volume += tick.last_size;
  acc.vwap_notional += tick.last * tick.last_size;
  acc.vwap_volume += tick.last_size;
  if (acc.vwap_volume > 0.0) {
    acc.bar.vwap = acc.vwap_notional / acc.vwap_volume;
  }
  acc.bar.ts_recv_ns = tick.ts_recv_ns;
}

void SimulatedFeed::start() {
  thread_ = std::jthread([this](std::stop_token st) { emit_loop(st); });
}

void SimulatedFeed::stop() {
  if (thread_.joinable()) {
    thread_.request_stop();
    thread_.join();
  }
}

void SimulatedFeed::subscribe(const Symbol &symbol,
                              std::optional<BarPeriod> period) {
  std::lock_guard lk{data_mu_};

  if (std::find(symbols_.begin(), symbols_.end(), symbol) == symbols_.end()) {
    symbols_.push_back(symbol);
  }

  if (period.has_value()) {
    const auto key = std::make_pair(symbol, period.value());
    ++bar_subscriptions_[key];

    auto [it, inserted] = accumulators_.try_emplace(key);
    if (inserted) {
      it->second.period_ns = period_to_ns(period.value());
    }
  } else {
    ++tick_subscriptions_[symbol];
  }
}

void SimulatedFeed::unsubscribe(const Symbol &symbol,
                                std::optional<BarPeriod> period) {
  std::lock_guard lk{data_mu_};

  if (period.has_value()) {
    const auto key = std::make_pair(symbol, period.value());
    auto sub_it = bar_subscriptions_.find(key);
    if (sub_it != bar_subscriptions_.end() && --sub_it->second == 0) {
      bar_subscriptions_.erase(sub_it);
      accumulators_.erase(key);
    }
  } else {
    auto sub_it = tick_subscriptions_.find(symbol);
    if (sub_it != tick_subscriptions_.end() && --sub_it->second == 0) {
      tick_subscriptions_.erase(sub_it);
    }
  }

  const bool still_has_bar_subs = std::any_of(
      bar_subscriptions_.begin(), bar_subscriptions_.end(),
      [&](const auto &entry) { return entry.first.first == symbol; });
  const bool still_has_tick_subs = tick_subscriptions_.contains(symbol);

  if (!still_has_bar_subs && !still_has_tick_subs) {
    symbols_.erase(std::remove(symbols_.begin(), symbols_.end(), symbol),
                   symbols_.end());
  }
}

void SimulatedFeed::set_bar_handler(std::function<void(const Bar &)> handler) {
  bar_handler_ = std::move(handler);
}

void SimulatedFeed::set_tick_handler(
    std::function<void(const Tick &)> handler) {
  tick_handler_ = std::move(handler);
}

void SimulatedFeed::emit_loop(std::stop_token st) {
  if (!tick_handler_ && !bar_handler_) [[unlikely]]
    return;

  std::mt19937_64 rng{std::random_device{}()};
  std::normal_distribution<double> noise{0.0, 0.10};

  std::unordered_map<Symbol, double> prices;

  while (!st.stop_requested()) {
    std::vector<Symbol> symbols;
    std::map<Symbol, int> tick_subscriptions;
    {
      std::lock_guard lk{data_mu_};
      symbols = symbols_;
      tick_subscriptions = tick_subscriptions_;
    }

    if (symbols.empty()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(350));
      continue;
    }

    const auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();

    for (const auto &sym : symbols) {
      double &price = prices[sym];
      if (price == 0.0) {
        price = 100.0;
      }
      price += noise(rng);
      if (price < 1.0) {
        price = 1.0;
      }

      Tick tick;
      tick.symbol = sym;
      tick.last = price;
      tick.bid = price - 0.01;
      tick.ask = price + 0.01;
      tick.bid_size = tick.ask_size = tick.last_size = 100.0;
      tick.ts_ns = now_ns;
      tick.ts_recv_ns = now_ns;

      std::vector<Bar> completed_bars;
      {
        std::lock_guard lk{data_mu_};
        for (auto &[key, acc] : accumulators_) {
          const auto &[bar_symbol, period] = key;
          if (bar_symbol != sym) {
            continue;
          }

          if (acc.bar_start_ns == 0) {
            start_bar(acc, bar_symbol, period, tick);
            continue;
          }

          if (tick.ts_ns >= acc.bar_start_ns + acc.period_ns.count()) {
            completed_bars.push_back(acc.bar);
            start_bar(acc, bar_symbol, period, tick);
          } else {
            update_bar(acc, tick);
          }
        }
      }

      if (tick_handler_ && tick_subscriptions.contains(sym)) {
        tick_handler_(tick);
      }

      if (bar_handler_) {
        for (const auto &bar : completed_bars) {
          bar_handler_(bar);
        }
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(350));
  }
}

} // namespace quarcc
