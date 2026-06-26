#include <trading/gateways/simulated_feed.h>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

namespace quarcc {

TEST(SimulatedFeedTest, DispatchesBarsByPeriodWithoutTicksForBarOnlySymbol) {
  SimulatedFeed feed;
  std::atomic<int> ticks{0};
  std::atomic<int> bars{0};

  feed.set_tick_handler([&](const Tick &) { ++ticks; });
  feed.set_bar_handler([&](const Bar &bar) {
    EXPECT_EQ(bar.symbol, "AAPL");
    EXPECT_EQ(bar.period, "1s");
    EXPECT_LE(bar.low, bar.high);
    EXPECT_GT(bar.volume, 0.0);
    ++bars;
  });

  feed.subscribe("AAPL", BarPeriod{"1s"});
  feed.start();
  std::this_thread::sleep_for(std::chrono::milliseconds(1600));
  feed.stop();

  EXPECT_EQ(ticks.load(), 0);
  EXPECT_GT(bars.load(), 0);
}

TEST(SimulatedFeedTest, DispatchesTicksOnlyWhenNoBarPeriodIsSubscribed) {
  SimulatedFeed feed;
  std::atomic<int> ticks{0};
  std::atomic<int> bars{0};

  feed.set_tick_handler([&](const Tick &tick) {
    EXPECT_EQ(tick.symbol, "MSFT");
    ++ticks;
  });
  feed.set_bar_handler([&](const Bar &) { ++bars; });

  feed.subscribe("MSFT", std::nullopt);
  feed.start();
  std::this_thread::sleep_for(std::chrono::milliseconds(800));
  feed.stop();

  EXPECT_GT(ticks.load(), 0);
  EXPECT_EQ(bars.load(), 0);
}

} // namespace quarcc
