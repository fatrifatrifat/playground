#include <trading/core/order_manager.h>

namespace quarcc {

std::unique_ptr<OrderManager> OrderManager::CreateOrderManager(
    std::unique_ptr<PositionKeeper> pk, std::unique_ptr<IExecutionGateway> gw,
    std::unique_ptr<IJournal> lj, std::unique_ptr<IOrderStore> os,
    std::unique_ptr<RiskManager> rm) {
  return std::unique_ptr<OrderManager>(
      new OrderManager(std::move(pk), std::move(gw), std::move(lj),
                       std::move(os), std::move(rm)));
}

Result<BrokerOrderId>
OrderManager::processSignal(const v1::StrategySignal &signal) {
  // Create order
  std::string local_id = id_generator_->generate();
  v1::Order order = createOrderFromSignal(signal);
  order.set_id(local_id);
  journal_->log(Event::ORDER_CREATED, order.DebugString(), order.id());

  StoredOrder stored;
  stored.order = order;
  stored.local_id = local_id;
  stored.status = OrderStatus::PENDING_SUBMISSION;
  stored.created_at = LogEntry::timestamp_to_string(LogEntry::now());

  if (auto store_result = order_store_->store_order(stored); !store_result) {
    journal_->log(Event::ERROR_OCCURRED, store_result.error().message_,
                  local_id);
    return std::unexpected(store_result.error());
  }

  // TODO: Risk check

  // Submit to gateway
  auto result = gateway_->submitOrder(order);
  if (!result) {
    journal_->log(Event::ORDER_REJECTED, result.error().message_, local_id);
    if (auto result =
            order_store_->update_order_status(local_id, OrderStatus::REJECTED);
        !result) {
      journal_->log(Event::ERROR_OCCURRED, result.error().message_, local_id);
      return std::unexpected(result.error());
    }
    return result;
  }

  std::string broker_id = result.value();
  if (auto result = order_store_->update_broker_id(local_id, broker_id);
      !result) {
    journal_->log(Event::ERROR_OCCURRED, result.error().message_, local_id);
    return std::unexpected(result.error());
  }

  id_mapper_->add_mapping(local_id, broker_id);

  std::string log_data = "Local: " + local_id + ", Broker: " + broker_id;
  journal_->log(Event::ORDER_SUBMITTED, log_data, local_id);

  if (auto result =
          order_store_->update_order_status(local_id, OrderStatus::SUBMITTED);
      !result) {
    journal_->log(Event::ERROR_OCCURRED, result.error().message_, local_id);
    return std::unexpected(result.error());
  }

  return local_id;
}

Result<std::monostate>
OrderManager::processSignal(const v1::CancelSignal &signal) {
  std::string local_id = signal.order_id();

  auto broker_id = id_mapper_->get_broker_id(local_id);
  if (!broker_id) {
    return std::unexpected(Error{"Cannot find broker ID for order: " + local_id,
                                 ErrorType::Error});
  }

  auto result = gateway_->cancelOrder(*broker_id);

  if (result) {
    journal_->log(Event::ORDER_CANCELLED, "Cancelled", local_id);
    if (auto result =
            order_store_->update_order_status(local_id, OrderStatus::CANCELLED);
        !result) {
      journal_->log(Event::ERROR_OCCURRED, result.error().message_, local_id);
      return std::unexpected(result.error());
    }
  }

  return result;
}

Result<BrokerOrderId>
OrderManager::processSignal(const v1::ReplaceSignal &signal) {
  v1::Order order = createOrderFromSignal(signal);
  auto result = gateway_->replaceOrder(signal.order_id(), order);
  if (!result) {
    return result;
  }
  return result;
}

OrderManager::OrderManager(std::unique_ptr<PositionKeeper> pk,
                           std::unique_ptr<IExecutionGateway> gw,
                           std::unique_ptr<IJournal> lj,
                           std::unique_ptr<IOrderStore> os,
                           std::unique_ptr<RiskManager> rm)
    : position_keeper_(std::move(pk)), gateway_(std::move(gw)),
      journal_(std::move(lj)), order_store_(std::move(os)),
      risk_manager_(std::move(rm)),
      id_generator_(std::make_unique<OrderIdGenerator>()),
      id_mapper_(std::make_unique<OrderIdMapper>()) {}

v1::Order
OrderManager::createOrderFromSignal(const v1::StrategySignal &signal) {
  v1::Order order;
  order.set_symbol(signal.symbol());
  order.set_side(signal.side());
  order.set_quantity(signal.target_quantity());
  order.set_type(v1::OrderType::MARKET);
  order.set_account_id("quarcc.Rifat");
  // order.created_at(order.id());
  order.set_time_in_force(v1::TimeInForce::DAY);
  order.set_strategy_id(signal.strategy_id());
  // order.metadata(). = signal.metadata(); // TODO: Copy metadata from signal
  return order;
}

v1::Order OrderManager::createOrderFromSignal(const v1::ReplaceSignal &signal) {
  v1::Order order;
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
