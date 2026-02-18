#pragma once

#include <cstdint>
#include <string>

namespace quarcc {

enum class ErrorType : std::uint8_t {
  Error,
  FailedOrder,
};

struct Error {
  std::string message_;
  ErrorType type_;
};

} // namespace quarcc
