#include <trading/core/trading_engine.h>

#include <thread>

namespace quarcc {

void TradingEngine::Run() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  gateway_ = std::make_shared<AlpacaGateway>();
  server_ = std::make_unique<gRPCServer>("0.0.0.0:50051", *this);

  std::thread server_thread{[this] {
    server_->start();
    server_->wait();
  }};

  server_->shutdown();
  server_thread.join();

  google::protobuf::ShutdownProtobufLibrary();
}

Result<OrderId> TradingEngine::SubmitSignal(const v1::StrategySignal &signal) {
  v1::Order order = createOrderFromSignal(signal);
  gateway_->submitOrder(order);
  return order.id();

  // Real version (roughly):
  // auto it = managers_.find(signal.strategy_id());
  // if (it == managers_.end()) return std::unexpected(Error{"Unknown strategy",
  // ErrorType::Error}); return it->second.ProcessSignal(signal); // validate +
  // persist + submit
}

Result<v1::Position>
TradingEngine::GetPosition(const v1::GetPositionRequest &) {
  return std::unexpected(
      Error{"GetPosition not implemented", ErrorType::Error});
}

Result<v1::PositionList> TradingEngine::GetAllPositions(const v1::Empty &) {
  return std::unexpected(
      Error{"GetAllPositions not implemented", ErrorType::Error});
}

Result<void> TradingEngine::ActivateKillSwitch(const v1::KillSwitchRequest &) {
  return std::unexpected(
      Error{"ActivateKillSwitch not implemented", ErrorType::Error});
}

v1::Order
TradingEngine::createOrderFromSignal(const v1::StrategySignal &signal) {
  v1::Order order;
  order.set_id(generateOrderId());
  order.set_symbol(signal.symbol());
  order.set_side(signal.side());
  order.set_quantity(signal.target_quantity());
  order.set_type(v1::OrderType::MARKET);
  order.set_account_id("quarcc.Rifat");
  // order.created_at(order.id()); // TODO: Timestamp
  order.set_time_in_force(v1::TimeInForce::DAY);
  order.set_strategy_id(signal.strategy_id());
  // order.metadata(). = signal.metadata(); // TODO: Copy metadata from signal
  return order;
}

} // namespace quarcc
