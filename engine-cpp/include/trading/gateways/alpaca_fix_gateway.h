#pragma once

#include <trading/interfaces/i_execution_gateway.h>

#include <alpaca/alpaca.hpp>

namespace quarcc {

class AlpacaGateway : public IExecutionGateway {
public:
  AlpacaGateway();

  void submitOrder(const v1::Order &order) override;
  void cancelOrder(const OrderId &orderId) override;
  void replaceOrder(const OrderId &orderId,
                    const v1::Order &new_order) override;

private:
  alpaca::Environment env_;
  alpaca::TradingClient trade_;
};

} // namespace quarcc
