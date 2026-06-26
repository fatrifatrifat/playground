#pragma once
#include <charconv>
#include <optional>
#include <string>

namespace quarcc::utils {

inline std::optional<int> str_to_int(std::string_view str) {
  int value{};
  const auto result = std::from_chars(str.data(), str.data() + str.size(), value);
  if (result.ec != std::errc() || result.ptr != str.data() + str.size()) {
    return std::nullopt;
  }

  return value;
}

}; // namespace quarcc::utils
