#pragma once

#include "execution_service.pb.h"
#include <trading/utils/result.h>

namespace quarcc {

struct IExecutionServiceHandler {
  virtual ~IExecutionServiceHandler() = default;

  virtual Result<OrderId> SubmitSignal(const v1::StrategySignal &req) = 0;
  virtual Result<v1::Position>
  GetPosition(const v1::GetPositionRequest &req) = 0;
  virtual Result<v1::PositionList> GetAllPositions(const v1::Empty &req) = 0;
  virtual Result<v1::Order> GetOrderStatus(const v1::GetOrderRequest &req) = 0;
  virtual Result<void> ActivateKillSwitch(const v1::KillSwitchRequest &req) = 0;
  // TODO: Stream services(?)
};

} // namespace quarcc
