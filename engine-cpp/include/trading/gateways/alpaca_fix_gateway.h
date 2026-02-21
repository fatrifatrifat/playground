#pragma once

#include <trading/interfaces/i_execution_gateway.h>

#include <alpaca/alpaca.hpp>

namespace quarcc {

class AlpacaGateway : public IExecutionGateway {
public:
  AlpacaGateway();

  Result<OrderId> submitOrder(const v1::Order &order) override;
  Result<std::monostate> cancelOrder(const OrderId &orderId) override;
  Result<OrderId> replaceOrder(const OrderId &orderId,
                               const v1::Order &new_order) override;

private:
  alpaca::Environment env_;
  alpaca::TradingClient trade_;
  OrderIdGenerator g;
};

} // namespace quarcc
