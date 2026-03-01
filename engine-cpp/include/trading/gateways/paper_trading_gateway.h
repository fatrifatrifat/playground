#pragma once

#include <trading/interfaces/i_execution_gateway.h>

namespace quarcc {

// Instant submission/cancelation/replacement/fills with custom local ids
class PaperGateway : public IExecutionGateway {
public:
  PaperGateway();

  Result<BrokerOrderId> submit_order(const v1::Order &order) override;
  Result<std::monostate> cancel_order(const BrokerOrderId &orderId) override;
  Result<BrokerOrderId> replace_order(const BrokerOrderId &orderId,
                                      const v1::Order &new_order) override;
  std::vector<v1::ExecutionReport> get_fills() override;

private:
private:
  std::unordered_map<BrokerOrderId, v1::Order> pending_orders_;
  std::mutex orders_mutex_;
  OrderIdGenerator id_gen_;
};

} // namespace quarcc
