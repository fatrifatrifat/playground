#include <trading/gateways/alpaca_fix_gateway.h>

namespace quarcc {

AlpacaGateway::AlpacaGateway() : trade_(env_) {}

void AlpacaGateway::submitOrder(const v1::Order &order) {
  std::cout << "Submit Order. Quantity: " << order.quantity() << '\n';
}

void AlpacaGateway::cancelOrder(const OrderId & /*orderId*/) {
  std::cout << "Cancel Order\n";
}

void AlpacaGateway::replaceOrder(const OrderId & /*orderId*/,
                                 const v1::Order & /*new_order*/) {
  std::cout << "Replace Order\n";
}

} // namespace quarcc
