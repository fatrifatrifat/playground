#pragma once

#include "execution_service.pb.h"

#include <trading/utils/order_id_generator.h>
#include <trading/utils/result.h>

#include <variant>

namespace quarcc {

struct IExecutionServiceHandler {
  virtual ~IExecutionServiceHandler() = default;

  virtual Result<BrokerOrderId> SubmitSignal(const v1::StrategySignal &req) = 0;
  virtual Result<std::monostate> CancelOrder(const v1::CancelSignal &req) = 0;
  virtual Result<BrokerOrderId> ReplaceOrder(const v1::ReplaceSignal &req) = 0;
  virtual Result<v1::Position>
  GetPosition(const v1::GetPositionRequest &req) = 0;
  virtual Result<v1::PositionList> GetAllPositions(const v1::Empty &req) = 0;
  virtual Result<std::monostate>
  ActivateKillSwitch(const v1::KillSwitchRequest &req) = 0;
  // TODO: Stream services(?)
};

} // namespace quarcc
