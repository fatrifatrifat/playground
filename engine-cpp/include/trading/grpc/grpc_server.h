#pragma once

#include "execution_service.grpc.pb.h"
#include "execution_service.pb.h"

#include <trading/utils/result.h>

namespace quarcc {

class gRPCServer {
  using StrategySignalCallback =
      std::move_only_function<Result<OrderId>(const v1::StrategySignal &)>;

public:
  gRPCServer(const std::string &server_address);

  void start();
  void wait();
  void shutdown();

  void setCallback(StrategySignalCallback &&callback);

private:
  Result<OrderId> InvokeCallback_(const v1::StrategySignal &signal);

  class ExecutionServiceImpl : public v1::ExecutionService::Service {
  public:
    ExecutionServiceImpl(gRPCServer *owner);

    grpc::Status SubmitSignal(grpc::ServerContext *context,
                              const v1::StrategySignal *request,
                              v1::SubmitSignalResponse *response) override;

    grpc::Status StreamSignals(
        grpc::ServerContext *context,
        grpc::ServerReaderWriter<v1::SubmitSignalResponse, v1::StrategySignal>
            *stream) override;

    grpc::Status GetPosition(grpc::ServerContext *context,
                             const v1::GetPositionRequest *request,
                             v1::Position *response) override;

    grpc::Status GetAllPositions(grpc::ServerContext *context,
                                 const v1::Empty *request,
                                 v1::PositionList *response) override;

    grpc::Status GetOrderStatus(grpc::ServerContext *context,
                                const v1::GetOrderRequest *request,
                                v1::Order *response) override;

    grpc::Status ActivateKillSwitch(grpc::ServerContext *context,
                                    const v1::KillSwitchRequest *request,
                                    v1::Empty *response) override;

  private:
    gRPCServer *owner_ = nullptr;
  };

  std::string server_address_;
  std::unique_ptr<ExecutionServiceImpl> service_;
  std::unique_ptr<grpc::Server> server_;
  StrategySignalCallback callback_;
};

} // namespace quarcc
