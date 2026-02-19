#pragma once

#include "strategy_signal.pb.h"

#include <trading/core/position_keeper.h>
#include <trading/interfaces/i_execution_gateway.h>
#include <trading/interfaces/i_journal.h>
#include <trading/interfaces/i_risk_check.h>
#include <trading/utils/order_id_generator.h>
#include <trading/utils/result.h>

namespace quarcc {

class OrderManager {
public:
  static std::unique_ptr<OrderManager> CreateOrderManager(
      std::unique_ptr<PositionKeeper> pk, std::unique_ptr<IExecutionGateway> gw,
      std::unique_ptr<LogJournal> lj, std::unique_ptr<RiskManager> rm);

  Result<OrderId> processSignal(const v1::StrategySignal &signal);

private:
  OrderManager(std::unique_ptr<PositionKeeper> pk,
               std::unique_ptr<IExecutionGateway> gw,
               std::unique_ptr<LogJournal> lj, std::unique_ptr<RiskManager> rm);

  v1::Order createOrderFromSignal(const v1::StrategySignal &signal);

private:
  std::unique_ptr<PositionKeeper> position_keeper_;
  std::unique_ptr<IExecutionGateway> gateway_;
  std::unique_ptr<LogJournal> journal_;
  std::unique_ptr<RiskManager> risk_manager_;
};

} // namespace quarcc
