#pragma once

#include <trading/core/order_manager.h>
#include <trading/gateways/alpaca_fix_gateway.h>
#include <trading/grpc/grpc_server.h>
#include <trading/interfaces/i_execution_service_handler.h>
#include <trading/utils/order_id_generator.h>

#include <memory>

namespace quarcc {

class TradingEngine final : public IExecutionServiceHandler {
  using StrategyId = std::string;

public:
  void Run();

private:
  Result<OrderId> SubmitSignal(const v1::StrategySignal &req) override;
  Result<std::monostate> CancelOrder(const v1::CancelSignal &req) override;
  Result<OrderId> ReplaceOrder(const v1::ReplaceSignal &req) override;
  Result<v1::Position> GetPosition(const v1::GetPositionRequest &req) override;
  Result<v1::PositionList> GetAllPositions(const v1::Empty &req) override;
  Result<std::monostate>
  ActivateKillSwitch(const v1::KillSwitchRequest &req) override;

private:
  std::atomic<bool> running_{true};
  std::unique_ptr<gRPCServer> server_;
  std::unordered_map<StrategyId, std::unique_ptr<OrderManager>> managers_;
};

} // namespace quarcc
