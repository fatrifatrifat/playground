#pragma once

#include <trading/utils/result.h>

#include <expected>
#include <vector>

namespace quarcc {

class IExecutionGateway {
public:
  virtual ~IExecutionGateway() {}

  virtual std::expected<std::string, Error> submitOrder();
};

} // namespace quarcc
