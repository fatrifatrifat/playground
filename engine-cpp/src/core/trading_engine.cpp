#include <chrono>
#include <iostream>
#include <random>
#include <thread>

#include <trading/core/trading_engine.h>

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

grpc::Status
StrategySignalGuideImpl::SendSignal(grpc::ServerContext * /*context*/,
                                    const v1::StrategySignal *signal,
                                    v1::Result *result) {
  std::cout << "[Server] Signal received\n";
  *result = processSignal(signal);
  std::cout << "[Server] Result sent\n";
  return grpc::Status::OK;
}

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  StrategySignalGuideImpl service;

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

StrategySignalClient::StrategySignalClient(
    std::shared_ptr<grpc::ChannelInterface> channel)
    : stub_(v1::StrategySignalGuide::NewStub(channel)) {}

void StrategySignalClient::SendSignal() {
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::default_random_engine generator(seed);
  std::uniform_int_distribution<int> confidence_distribution(0, 100);

  v1::StrategySignal signal;
  signal.set_confidence(confidence_distribution(generator));

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

  std::thread server{RunServer};

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
