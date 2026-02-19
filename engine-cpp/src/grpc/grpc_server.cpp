#include <grpcpp/grpcpp.h>
#include <trading/grpc/grpc_server.h>

namespace quarcc {

gRPCServer::gRPCServer(const std::string &server_address)
    : server_address_(server_address) {}

void gRPCServer::start() {
  service_ = std::make_unique<ExecutionServiceImpl>(this);

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address_, grpc::InsecureServerCredentials());
  builder.RegisterService(service_.get());

  server_ = builder.BuildAndStart();
  std::cout << "gRPC server listening on " << server_address_ << std::endl;
}
void gRPCServer::wait() { server_->Wait(); }
void gRPCServer::shutdown() {
  if (server_)
    server_->Shutdown();
}

void gRPCServer::setCallback(StrategySignalCallback &&callback) {
  callback_ = std::move(callback);
}

Result<OrderId> gRPCServer::InvokeCallback_(const v1::StrategySignal &signal) {
  if (callback_)
    return callback_(signal);

  return std::unexpected(Error{"Callback does not exist", ErrorType::Error});
}

gRPCServer::ExecutionServiceImpl::ExecutionServiceImpl(gRPCServer *owner)
    : owner_(owner) {}

grpc::Status gRPCServer::ExecutionServiceImpl::SubmitSignal(
    grpc::ServerContext *context, const v1::StrategySignal *request,
    v1::SubmitSignalResponse *response) {
  std::cout << "Received signal from " << context->peer() << " - "
            << request->strategy_id() << " " << request->side() << " "
            << request->instrument().symbol() << std::endl;

  if (!owner_) {
    response->set_accepted(false);
    response->set_rejection_reason("No owner of the service");
  }

  // Submit to order manager
  const auto result = owner_->InvokeCallback_(*request);
  if (!result) {
    response->set_accepted(false);
    response->set_rejection_reason(result.error().message_);
  }

  std::cout << "Signal accepted, order ID: " << result.value() << std::endl;
  response->set_accepted(true);
  return grpc::Status::OK;
}

grpc::Status gRPCServer::ExecutionServiceImpl::StreamSignals(
    grpc::ServerContext *context,
    grpc::ServerReaderWriter<v1::SubmitSignalResponse, v1::StrategySignal>
        *stream) {}

grpc::Status gRPCServer::ExecutionServiceImpl::GetPosition(
    grpc::ServerContext *context, const v1::GetPositionRequest *request,
    v1::Position *response) {}

grpc::Status
gRPCServer::ExecutionServiceImpl::GetAllPositions(grpc::ServerContext *context,
                                                  const v1::Empty *request,
                                                  v1::PositionList *response) {}

grpc::Status gRPCServer::ExecutionServiceImpl::GetOrderStatus(
    grpc::ServerContext *context, const v1::GetOrderRequest *request,
    v1::Order *response) {}

grpc::Status gRPCServer::ExecutionServiceImpl::ActivateKillSwitch(
    grpc::ServerContext *context, const v1::KillSwitchRequest *request,
    v1::Empty *response) {}

} // namespace quarcc
