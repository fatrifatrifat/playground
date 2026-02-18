#include <iostream>
#include <thread>

#include <glob.h>
#include <trading/core/trading_engine.h>

namespace quarcc {

StrategySignalClient::StrategySignalClient(
    std::shared_ptr<grpc::ChannelInterface> channel)
    : stub_(v1::StrategySignalGuide::NewStub(channel)) {}

void StrategySignalClient::SendSignal() {
  static int qty = 0;
  v1::StrategySignal signal;
  signal.set_order_qty_e8(qty++);
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
  signal_source_->setCallback([this](const v1::StrategySignal &signal) {
    gateway_->submitOrder(createOrderFromSignal(signal));
    v1::Result res;
    res.set_state(v1::State::ACCEPTED);
    return res;
  });

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

v1::Order
TradingEngine::createOrderFromSignal(const v1::StrategySignal &signal) {
  v1::Order order;

  order.set_id(generateOrderId());
  order.set_symbol(signal.instrument().symbol());
  order.set_side(signal.side());
  order.set_quantity(signal.order_qty_e8());
  order.set_order_type(v1::OrdType::ORD_MARKET); // TODO: Based on config file
  // order.set_price() // TODO: Price based on best in memory
  order.set_account_id("quarcc.Rifat"); // TODO: Based on config file
  order.set_timestamp(order.id());
  order.set_time_in_force(
      v1::TimeInForce::TIF_DAY); // TODO: Cancel at the end of the day
  order.set_strategy_id(signal.strategy_id());
  order.mutable_meta()->CopyFrom(signal.meta());

  return order;
}

} // namespace quarcc
