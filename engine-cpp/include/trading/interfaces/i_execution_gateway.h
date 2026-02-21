#pragma once

#include <trading/utils/order_id_generator.h>
#include <trading/utils/result.h>

#include "order.pb.h"

namespace quarcc {

class IExecutionGateway {
public:
  virtual ~IExecutionGateway() = default;

  virtual Result<OrderId> submitOrder(const v1::Order &order) = 0;
  virtual Result<std::monostate> cancelOrder(const OrderId &orderId) = 0;
  virtual Result<OrderId> replaceOrder(const OrderId &orderId,
                            const v1::Order &new_order) = 0;
};

} // namespace quarcc
