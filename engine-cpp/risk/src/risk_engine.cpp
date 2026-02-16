#include "club/risk/risk_engine.hpp"

namespace club::risk {

bool RiskEngine::allow(const club::trading::v1::NewOrderIntent& intent, std::string& reason) const {
  if (intent.qty_e8() <= 0) {
    reason = "QTY_NON_POSITIVE";
    return false;
  }
  if (intent.qty_e8() > max_order_qty_e8_) {
    reason = "MAX_ORDER_QTY_BREACH";
    return false;
  }
  reason.clear();
  return true;
}

} // namespace club::risk
