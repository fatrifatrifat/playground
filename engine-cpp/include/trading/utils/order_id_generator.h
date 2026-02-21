#pragma once

#include <atomic>
#include <chrono>
#include <format>

namespace quarcc {

using OrderId = std::string;

// Is there really a point to generating ids for orders if the exchange/broker
// already gives us ids for them when created
class OrderIdGenerator {
public:
  OrderId generate() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                         now.time_since_epoch())
                         .count();

    return std::format("ORD_{}_{:06d}", timestamp, counter_++);
  }

private:
  std::atomic<std::uint64_t> counter_{0};
};

inline std::string get_current_time() noexcept {
  const auto now = std::chrono::system_clock::now();
  return std::format("{:%FT%TZ}", now);
}

} // namespace quarcc
