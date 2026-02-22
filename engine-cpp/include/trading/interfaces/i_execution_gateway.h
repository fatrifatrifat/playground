#pragma once

#include <trading/utils/order_id_generator.h>
#include <trading/utils/result.h>

#include "order.pb.h"

namespace quarcc {

class IExecutionGateway {
public:
  virtual ~IExecutionGateway() = default;

  virtual Result<BrokerOrderId> submitOrder(const v1::Order &order) = 0;
  virtual Result<std::monostate> cancelOrder(const BrokerOrderId &orderId) = 0;
  virtual Result<BrokerOrderId> replaceOrder(const BrokerOrderId &orderId,
                                             const v1::Order &new_order) = 0;
};

} // namespace quarcc
