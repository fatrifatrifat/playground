#pragma once

#include <grpc/grpc.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/server_builder.h>

#include "strategy_signal.grpc.pb.h"
#include "strategy_signal.pb.h"

#include <memory>

namespace quarcc {
// Declare only
void RunServer();

class StrategySignalGuideImpl final : public v1::StrategySignalGuide::Service {
public:
  grpc::Status SendSignal(grpc::ServerContext *context,
                          const v1::StrategySignal *signal,
                          v1::Result *result) override;
};

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
};

} // namespace quarcc
