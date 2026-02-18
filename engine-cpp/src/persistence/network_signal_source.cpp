#include <trading/persistence/network_signal_source.h>

namespace quarcc {

NetworkSignalSource::NetworkSignalSource() : service_(this) {}
void NetworkSignalSource::start() { RunServer(); }
void NetworkSignalSource::stop() { server_->Shutdown(); }
void NetworkSignalSource::setCallback(
    NetworkSignalSource::SignalCallback callback) {
  callback_ = std::move(callback);
}

void NetworkSignalSource::RunServer() {
  std::string server_address("0.0.0.0:50051");

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service_);
  server_ = builder.BuildAndStart();
  std::cout << "Server listening on " << server_address << std::endl;
  server_->Wait();
}

void NetworkSignalSource::InvokeCallback_() {
  if (callback_)
    callback_();
}

NetworkSignalSource::StrategySignalGuideImpl::StrategySignalGuideImpl(
    NetworkSignalSource *owner)
    : owner_(owner) {}

grpc::Status NetworkSignalSource::StrategySignalGuideImpl::SendSignal(
    grpc::ServerContext * /*context*/, const v1::StrategySignal * /*signal*/,
    v1::Result * /*result*/) {

  std::cout << "[Server] Signal received\n";

  if (owner_)
    owner_->InvokeCallback_();

  std::cout << "[Server] Result sent\n";
  return grpc::Status::OK;
}

} // namespace quarcc
