#include <trading/gateways/alpaca_fix_gateway.h>

namespace quarcc {

AlpacaGateway::AlpacaGateway() : trade_(env_) {}

Result<BrokerOrderId> AlpacaGateway::submitOrder(const v1::Order &order) {
  std::cout << "Submit Order. Quantity: " << order.id() << ", quantity"
            << order.quantity() << '\n';
  // Return ID from broker
  return g.generate();
}

Result<std::monostate>
AlpacaGateway::cancelOrder(const BrokerOrderId &orderId) {
  std::cout << "Cancel Order, ID: " << orderId << '\n';
  return std::monostate{};
}

Result<BrokerOrderId> AlpacaGateway::replaceOrder(const BrokerOrderId &orderId,
                                                  const v1::Order &new_order) {
  std::cout << "Replace Order, ID: " << orderId << " replaced by "
            << new_order.id() << ", quantity: " << new_order.quantity() << '\n';
  // Return ID from broker
  return g.generate();
}

} // namespace quarcc
