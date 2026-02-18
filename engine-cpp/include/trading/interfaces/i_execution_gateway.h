#pragma once

#include <trading/utils/result.h>

#include "strategy_signal.grpc.pb.h"
#include "strategy_signal.pb.h"

#include <variant>

namespace quarcc {

class IExecutionGateway {
public:
  virtual ~IExecutionGateway() = default;

  virtual void submitOrder() = 0;
  virtual void cancelOrder() = 0;
  virtual void replaceOrder() = 0;
};

} // namespace quarcc
