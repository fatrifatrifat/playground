#pragma once

#include <trading/interfaces/i_signal_source.h>

namespace quarcc {

class NetworkSignalSource : public ISignalSource {
public:
  NetworkSignalSource();
  void start() override;
  void stop() override;
  void setCallback(SignalCallback callback) override;

private:
  void RunServer();
  void InvokeCallback_();

  class StrategySignalGuideImpl final
      : public v1::StrategySignalGuide::Service {
  public:
    explicit StrategySignalGuideImpl(NetworkSignalSource *owner);

    grpc::Status SendSignal(grpc::ServerContext * /*context*/,
                            const v1::StrategySignal *signal,
                            v1::Result *result) override;

  private:
    NetworkSignalSource *owner_ = nullptr;
  };

  StrategySignalGuideImpl service_;
  std::unique_ptr<grpc::Server> server_;
};

} // namespace quarcc
