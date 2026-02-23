#include <trading/gateways/alpaca_fix_gateway.h>

namespace quarcc {

AlpacaGateway::AlpacaGateway() : trade_(env_) {}

Result<BrokerOrderId> AlpacaGateway::submit_order(const v1::Order &order) {
  const auto o = order_to_alpaca_order(order);
  auto resp = trade_.SubmitOrder(o);
  if (!resp) {
    return std::unexpected(Error{resp.error().message, ErrorType::Error});
  }

  const std::string &broker_id = resp->id;

  {
    std::lock_guard lk{orders_mutex_};
    pending_orders_[broker_id] = order;
  }

  return broker_id;
}

Result<std::monostate>
AlpacaGateway::cancel_order(const BrokerOrderId &orderId) {
  auto resp = trade_.GetOrderByID(orderId);
  if (!resp) {
    return std::unexpected(Error{resp.error().message, ErrorType::Error});
  }

  {
    std::lock_guard lk{orders_mutex_};
    pending_orders_.erase(orderId);
  }

  return std::monostate{};
}

Result<BrokerOrderId> AlpacaGateway::replace_order(const BrokerOrderId &orderId,
                                                   const v1::Order &new_order) {
  const alpaca::ReplaceOrderParam replace{
      .qty = std::make_optional(new_order.quantity()),
  };

  auto resp = trade_.ReplaceOrderByID(orderId, replace);
  if (!resp) {
    return std::unexpected(Error{resp.error().message, ErrorType::Error});
  }

  const std::string &broker_id = resp->id;
  {
    std::lock_guard lk{orders_mutex_};
    pending_orders_.erase(orderId);
    pending_orders_[broker_id] = new_order;
  }

  return broker_id;
}

std::vector<v1::ExecutionReport> AlpacaGateway::get_fills() {
  std::vector<v1::ExecutionReport> fills;

  std::lock_guard lk{orders_mutex_};

  for (const auto &[broker_id, _] : pending_orders_) {
    auto resp = trade_.GetOrderByID(broker_id);
    if (!resp) {
      continue;
    }

    const auto &order = resp.value();
    v1::ExecutionReport fill;
    fill.set_broker_order_id(broker_id);
    fill.set_symbol(order.symbol.value());
    fill.set_side((order.side == alpaca::OrderSide::buy) ? v1::Side::BUY
                                                         : v1::Side::SELL);
    fill.set_filled_quantity(stod(order.filledQty));
    fill.set_fill_time(get_current_time());
    fills.push_back(std::move(fill));

    pending_orders_.erase(broker_id);
  }

  return fills;
}

constexpr alpaca::OrderRequestParam
AlpacaGateway::order_to_alpaca_order(const v1::Order &order) const {
  return alpaca::OrderRequestParam{
      .symbol = order.symbol(),
      .amt = alpaca::ShareAmount{alpaca::Quantity{order.quantity()}},
      .side = order_enum_conversion(order.side()),
      .type = order_enum_conversion(order.type()),
      .timeInForce = order_enum_conversion(order.time_in_force()),
  };
}

constexpr alpaca::OrderSide
AlpacaGateway::order_enum_conversion(v1::Side type) const {
  switch (static_cast<int>(type)) {
  case v1::Side::BUY:
    return alpaca::OrderSide::buy;
  case v1::Side::SELL:
    return alpaca::OrderSide::sell;
  }
}

constexpr alpaca::OrderType
AlpacaGateway::order_enum_conversion(v1::OrderType type) const {
  switch (static_cast<int>(type)) {
  case v1::OrderType::MARKET:
    return alpaca::OrderType::market;
  case v1::OrderType::LIMIT:
    return alpaca::OrderType::limit;
  case v1::OrderType::STOP_LIMIT:
    return alpaca::OrderType::stop_limit;
  case v1::OrderType::STOP:
    return alpaca::OrderType::stop;
  }
}

constexpr alpaca::OrderTimeInForce
AlpacaGateway::order_enum_conversion(v1::TimeInForce type) const {
  switch (static_cast<int>(type)) {
  case v1::TimeInForce::DAY:
    return alpaca::OrderTimeInForce::day;
  case v1::TimeInForce::FOK:
    return alpaca::OrderTimeInForce::fok;
  case v1::TimeInForce::GTC:
    return alpaca::OrderTimeInForce::gtc;
  case v1::TimeInForce::IOC:
    return alpaca::OrderTimeInForce::ioc;
  }
}

} // namespace quarcc
