#include <trading/core/trading_engine.h>

#include <thread>

namespace quarcc {

void TradingEngine::Run() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  managers_.emplace(
      StrategyId{"SMA_CROSS_v1.0"},
      OrderManager::CreateOrderManager(
          std::make_unique<PositionKeeper>(), std::make_unique<AlpacaGateway>(),
          std::make_unique<LogJournal>(), std::make_unique<RiskManager>()));

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
  auto it = managers_.find(signal.strategy_id());
  if (it == managers_.end())
    return std::unexpected(Error{"Unknown strategy", ErrorType::Error});
  return it->second->processSignal(signal);
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

} // namespace quarcc
