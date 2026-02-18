#include <chrono>
#include <iostream>
#include <random>
#include <thread>

#include <trading/core/trading_engine.h>

namespace quarcc {

StrategySignalClient::StrategySignalClient(
    std::shared_ptr<grpc::ChannelInterface> channel)
    : stub_(v1::StrategySignalGuide::NewStub(channel)) {}

void StrategySignalClient::SendSignal() {
  v1::StrategySignal signal;
  v1::Result result;

  grpc::ClientContext context;
  grpc::Status status = stub_->SendSignal(&context, signal, &result);
  if (!status.ok()) {
    std::cout << "[Client] SendSignal rpc failed.\n";
    return;
  }
  std::cout << "[Client] state: " << result.state() << "\n";
}

void TradingEngine::Run() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  signal_source_ = std::make_unique<NetworkSignalSource>();
  gateway_ = std::make_shared<AlpacaGateway>();
  signal_source_->setCallback([this] { gateway_->submitOrder(); });

  std::thread server{&ISignalSource::start, signal_source_.get()};

  StrategySignalClient guide(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials()));

  std::thread client{[&guide] {
    using namespace std::chrono_literals;

    for (int i = 0; i < 100; ++i) {
      guide.SendSignal();
      std::this_thread::sleep_for(5s);
    }
  }};

  client.join();
  server.join();

  google::protobuf::ShutdownProtobufLibrary();
}

} // namespace quarcc
