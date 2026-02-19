#include <trading/core/order_manager.h>

namespace quarcc {

std::unique_ptr<OrderManager> OrderManager::CreateOrderManager(
    std::unique_ptr<PositionKeeper> pk, std::unique_ptr<IExecutionGateway> gw,
    std::unique_ptr<LogJournal> lj, std::unique_ptr<RiskManager> rm) {
  return std::unique_ptr<OrderManager>(new OrderManager(
      std::move(pk), std::move(gw), std::move(lj), std::move(rm)));
}

Result<OrderId> OrderManager::processSignal(const v1::StrategySignal &signal) {
  v1::Order order = createOrderFromSignal(signal);
  gateway_->submitOrder(order);
  return order.id();
}

OrderManager::OrderManager(std::unique_ptr<PositionKeeper> pk,
                           std::unique_ptr<IExecutionGateway> gw,
                           std::unique_ptr<LogJournal> lj,
                           std::unique_ptr<RiskManager> rm)
    : position_keeper_(std::move(pk)), gateway_(std::move(gw)),
      journal_(std::move(lj)), risk_manager_(std::move(rm)) {}

v1::Order
OrderManager::createOrderFromSignal(const v1::StrategySignal &signal) {
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
