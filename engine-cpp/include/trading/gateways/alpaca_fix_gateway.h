#pragma once

#include <trading/interfaces/i_execution_gateway.h>

#include <alpaca/alpaca.hpp>

namespace quarcc {

class AlpacaGateway : public IExecutionGateway {
public:
  AlpacaGateway();

  void submitOrder() override;
  void cancelOrder() override;
  void replaceOrder() override;

private:
  alpaca::Environment env_;
  alpaca::TradingClient trade_;
};

} // namespace quarcc
