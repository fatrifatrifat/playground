#pragma once

#include <trading/gateways/alpaca_fix_gateway.h>
#include <trading/persistence/network_signal_source.h>
#include <trading/utils/order_id_generator.h>

#include <memory>

namespace quarcc {

class StrategySignalClient {
public:
  explicit StrategySignalClient(
      std::shared_ptr<grpc::ChannelInterface> channel);
  void SendSignal();

private:
  std::unique_ptr<v1::StrategySignalGuide::Stub> stub_;
};

class TradingEngine {
public:
  void Run();

private:
  v1::Order createOrderFromSignal(const v1::StrategySignal &signal);

  std::unique_ptr<ISignalSource> signal_source_;
  std::shared_ptr<IExecutionGateway> gateway_;
};

} // namespace quarcc
