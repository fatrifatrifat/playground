#pragma once
#include <cstddef>
#include <spdlog/spdlog.h>
#include <trading/utils/result.h>
#include <variant>

namespace quarcc {

struct RiskLimits {
  // If value is 0 => disabled

  // Max quantity in a single order
  double max_quantity{0.};
  // Max amount of open orders at the same time
  size_t max_open_orders{0uz};
  // Accepted daily loss
  double daily_pnl_limit{0.};
};

class RiskManager {
public:
  RiskManager(RiskLimits limits) : limits_(limits) {}
  Result<std::monostate> check(double qty, size_t open_order, double pnl) {
    if (limits_.max_quantity && qty > limits_.max_quantity) {
      return std::unexpected(Error{
          .message_ = "Max quantity limit exceeded",
          .type_ = ErrorType::FailedOrder,
      });
    }

    if (limits_.max_open_orders && open_order > limits_.max_open_orders) {
      return std::unexpected(Error{
          .message_ = "Max open order amount limit exceeded",
          .type_ = ErrorType::FailedOrder,
      });
    }

    if (limits_.daily_pnl_limit && pnl < -limits_.daily_pnl_limit) {
      return std::unexpected(Error{
          .message_ = "Daily loss limit exceeded",
          .type_ = ErrorType::FailedOrder,
      });
    }

    return std::monostate{};
  }

private:
  RiskLimits limits_;
};

}; // namespace quarcc
