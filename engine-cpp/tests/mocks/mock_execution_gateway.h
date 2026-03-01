#pragma once

#include <gmock/gmock.h>
#include <trading/interfaces/i_execution_gateway.h>

namespace quarcc {

class MockExecutionGateway : public IExecutionGateway {
public:
  MOCK_METHOD(Result<BrokerOrderId>, submit_order, (const v1::Order &order),
              (override));
  MOCK_METHOD(Result<std::monostate>, cancel_order,
              (const BrokerOrderId &orderId), (override));
  MOCK_METHOD(Result<BrokerOrderId>, replace_order,
              (const BrokerOrderId &orderId, const v1::Order &new_order),
              (override));
  MOCK_METHOD(std::vector<v1::ExecutionReport>, get_fills, (), (override));
};

} // namespace quarcc
