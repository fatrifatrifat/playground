#pragma once

#include <trading/persistence/network_signal_source.h>

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
  std::unique_ptr<ISignalSource> signal_source_;
};

} // namespace quarcc
