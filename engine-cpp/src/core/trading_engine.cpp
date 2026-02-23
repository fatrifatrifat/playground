#include <trading/core/trading_engine.h>
#include <trading/persistence/sqlite_journal.h>
#include <trading/persistence/sqlite_order_store.h>

namespace quarcc {

void TradingEngine::Run() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  // Initializing managers
  managers_.emplace(
      StrategyId{"SMA_CROSS_v1.0"},
      OrderManager::CreateOrderManager(
          std::make_unique<PositionKeeper>(), std::make_unique<AlpacaGateway>(),
          std::make_unique<SQLiteJournal>("SMA_CROSS_v1_trading_journal.db"),
          std::make_unique<SQLiteOrderStore>("SMA_CROSS_v1_trading_orders.db"),
          std::make_unique<RiskManager>()));

  // Setting up the server
  server_ = std::make_unique<gRPCServer>("0.0.0.0:50051", *this);
  server_->start();

  while (running_) {
    // Process fills
    // Update metrics
  }

  server_->shutdown();
  google::protobuf::ShutdownProtobufLibrary();
}

Result<BrokerOrderId>
TradingEngine::SubmitSignal(const v1::StrategySignal &signal) {
  auto it = managers_.find(signal.strategy_id());
  if (it == managers_.end())
    return std::unexpected(Error{"Unknown strategy", ErrorType::Error});
  return it->second->processSignal(signal);
}

Result<std::monostate>
TradingEngine::CancelOrder(const v1::CancelSignal &signal) {
  auto it = managers_.find(signal.strategy_id());
  if (it == managers_.end())
    return std::unexpected(Error{"Unknown strategy", ErrorType::Error});
  return it->second->processSignal(signal);
}

Result<BrokerOrderId>
TradingEngine::ReplaceOrder(const v1::ReplaceSignal &signal) {
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

Result<std::monostate>
TradingEngine::ActivateKillSwitch(const v1::KillSwitchRequest &) {
  return std::unexpected(
      Error{"ActivateKillSwitch not implemented", ErrorType::Error});
}

} // namespace quarcc
