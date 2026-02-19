#pragma once

#include "order.pb.h"
#include "strategy_signal.pb.h"

#include <trading/utils/result.h>

#include <chrono>
#include <format>

namespace quarcc {

inline OrderId generateOrderId() {
  const auto now = std::chrono::system_clock::now();
  return std::format("{:%FT%TZ}", now);
}

} // namespace quarcc
