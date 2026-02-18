#include <trading/persistence/network_signal_source.h>

namespace quarcc {

static v1::Result processSignal(const v1::StrategySignal *signal) {
  std::cout << "Processing signal, value: " << signal->confidence() << '\n';

  v1::Result res;
  if (signal->confidence() >= 80.) {
    res.set_state(v1::State::ACCEPTED);
  } else if (signal->confidence() >= 60.) {
    res.set_state(v1::State::WAITING);
  } else {
    res.set_state(v1::State::DECLINED);
  }
  return res;
}

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

grpc::Status NetworkSignalSource::StrategySignalGuideImpl::SendSignal(
    grpc::ServerContext * /*context*/, const v1::StrategySignal *signal,
    v1::Result *result) {
  std::cout << "[Server] Signal received\n";
  *result = processSignal(signal);
  std::cout << "[Server] Result sent\n";
  return grpc::Status::OK;
}

} // namespace quarcc
