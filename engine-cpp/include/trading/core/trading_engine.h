#pragma once

#include <trading/gateways/alpaca_fix_gateway.h>
#include <trading/grpc/grpc_server.h>
#include <trading/interfaces/i_execution_service_handler.h>
#include <trading/utils/order_id_generator.h>

#include <memory>

namespace quarcc {

class ExecutionServiceClient {
public:
  explicit ExecutionServiceClient(
      std::shared_ptr<grpc::ChannelInterface> channel);
  void SubmitSignal();

private:
  std::unique_ptr<v1::ExecutionService::Stub> stub_;
};

class TradingEngine final : public IExecutionServiceHandler {
public:
  void Run();

  Result<OrderId> SubmitSignal(const v1::StrategySignal &req) override;
  Result<v1::Position> GetPosition(const v1::GetPositionRequest &req) override;
  Result<v1::PositionList> GetAllPositions(const v1::Empty &req) override;
  Result<v1::Order> GetOrderStatus(const v1::GetOrderRequest &req) override;
  Result<void> ActivateKillSwitch(const v1::KillSwitchRequest &req) override;

private:
  v1::Order createOrderFromSignal(const v1::StrategySignal &signal);

  std::shared_ptr<IExecutionGateway> gateway_;
  std::unique_ptr<gRPCServer> server_;

  // std::map<StrategyId, OrderManager> managers_;  // your real data
};

} // namespace quarcc
