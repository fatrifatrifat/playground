#pragma once

#include <trading/gateways/alpaca_fix_gateway.h>
#include <trading/grpc/grpc_server.h>
#include <trading/persistence/network_signal_source.h>
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

class TradingEngine {
public:
  void Run();

private:
  v1::Order createOrderFromSignal(const v1::StrategySignal &signal);

  std::shared_ptr<IExecutionGateway> gateway_;
  std::unique_ptr<gRPCServer> server_;
};

} // namespace quarcc
