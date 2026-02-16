#pragma once
#include <string>
#include "order.pb.h"

namespace club::risk {

class RiskEngine {
public:
  explicit RiskEngine(long long max_order_qty_e8) : max_order_qty_e8_(max_order_qty_e8) {}

  bool allow(const club::trading::v1::NewOrderIntent& intent, std::string& reason) const;

private:
  long long max_order_qty_e8_;
};

} // namespace club::risk
