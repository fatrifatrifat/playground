#include <iostream>
#include <thread>

#include <trading/core/trading_engine.h>

namespace quarcc {

ExecutionServiceClient::ExecutionServiceClient(
    std::shared_ptr<grpc::ChannelInterface> channel)
    : stub_(v1::ExecutionService::NewStub(channel)) {}

void ExecutionServiceClient::SubmitSignal() {
  static int qty = 0;
  v1::StrategySignal signal;
  signal.set_order_qty_e8(qty++);
  v1::SubmitSignalResponse result;

  grpc::ClientContext context;
  grpc::Status status = stub_->SubmitSignal(&context, signal, &result);
  if (!status.ok()) {
    std::cout << "[Client] SubmitSignal rpc failed.\n";
    return;
  }

  if (result.accepted()) {
    std::cout << "[Client] Signal Accepted\n";
    return;
  }

  std::cout << "[Client] Signal Declined. Reason: " << result.rejection_reason()
            << std::endl;
}

void TradingEngine::Run() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  gateway_ = std::make_shared<AlpacaGateway>();
  server_ = std::make_unique<gRPCServer>("0.0.0.0:50051");

  server_->setCallback([this](const v1::StrategySignal &signal) {
    v1::Order order = createOrderFromSignal(signal);
    gateway_->submitOrder(order);
    return order.id();
  });

  std::thread server{[this] {
    server_->start();
    server_->wait();
  }};

  ExecutionServiceClient guide(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials()));

  std::thread client{[&guide] {
    using namespace std::chrono_literals;

    for (int i = 0; i < 100; ++i) {
      guide.SubmitSignal();
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
