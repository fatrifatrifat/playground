#pragma once

#include <trading/utils/order_id_generator.h>
#include <trading/utils/result.h>

#include "execution.pb.h"
#include "order.pb.h"

namespace quarcc {

class IExecutionGateway {
public:
  virtual ~IExecutionGateway() = default;

  virtual Result<BrokerOrderId> submit_order(const v1::Order &order) = 0;
  virtual Result<std::monostate> cancel_order(const BrokerOrderId &orderId) = 0;
  virtual Result<BrokerOrderId> replace_order(const BrokerOrderId &orderId,
                                              const v1::Order &new_order) = 0;
  virtual std::vector<v1::ExecutionReport> get_fills() = 0;
};

} // namespace quarcc
