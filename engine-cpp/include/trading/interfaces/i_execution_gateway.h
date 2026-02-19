#pragma once

#include <trading/utils/result.h>

#include "order.pb.h"

namespace quarcc {

class IExecutionGateway {
public:
  virtual ~IExecutionGateway() = default;

  virtual void submitOrder(const v1::Order &order) = 0;
  virtual void cancelOrder(const OrderId &orderId) = 0;
  virtual void replaceOrder(const OrderId &orderId,
                            const v1::Order &new_order) = 0;
};

} // namespace quarcc
