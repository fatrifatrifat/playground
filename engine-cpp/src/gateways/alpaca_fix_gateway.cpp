#include <trading/gateways/alpaca_fix_gateway.h>

namespace quarcc {

AlpacaGateway::AlpacaGateway() : trade_(env_) {}

void AlpacaGateway::submitOrder() { std::cout << "Submit Order\n"; }

void AlpacaGateway::cancelOrder() { std::cout << "Cancel Order\n"; }

void AlpacaGateway::replaceOrder() { std::cout << "Replace Order\n"; }

} // namespace quarcc
