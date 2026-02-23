#pragma once

#include <trading/interfaces/i_execution_gateway.h>

#include <alpaca/alpaca.hpp>

namespace quarcc {

class AlpacaGateway : public IExecutionGateway {
public:
  AlpacaGateway();

  // TODO: Make config file for API key parsing
  // TODO: Fix warnings, probably by giving default value as std::nullopt to
  // alpaca-sdk structs
  Result<BrokerOrderId> submit_order(const v1::Order &order) override;
  Result<std::monostate> cancel_order(const BrokerOrderId &orderId) override;
  Result<BrokerOrderId> replace_order(const BrokerOrderId &orderId,
                                      const v1::Order &new_order) override;
  std::vector<v1::ExecutionReport> get_fills() override;

private:
  constexpr alpaca::OrderRequestParam
  order_to_alpaca_order(const v1::Order &order) const;
  constexpr alpaca::OrderSide order_enum_conversion(v1::Side type) const;
  constexpr alpaca::OrderType order_enum_conversion(v1::OrderType type) const;
  constexpr alpaca::OrderTimeInForce
  order_enum_conversion(v1::TimeInForce type) const;

private:
  alpaca::Environment env_;
  alpaca::TradingClient trade_;

  std::unordered_map<BrokerOrderId, v1::Order> pending_orders_;
  std::mutex orders_mutex_;
};

} // namespace quarcc
