#pragma once

#include <chrono>
#include <format>

namespace quarcc {

using OrderId = std::string;

inline std::string getCurrentTime() {
  const auto now = std::chrono::system_clock::now();
  return std::format("{:%FT%TZ}", now);
}

} // namespace quarcc
