#include <iostream>
#include <string>

#include "club/core/engine.hpp"
#include "club/risk/risk_engine.hpp"
#include "order.pb.h"

int main() {
  club::core::Engine engine;
  engine.start();

  club::trading::v1::NewOrderIntent intent;
  intent.set_strategy_id("demo_stat_arb");
  intent.set_qty_e8(50'00000000LL); // 50.0 units

  club::risk::RiskEngine risk(/*max_order_qty_e8=*/100'00000000LL); // 100.0 units
  std::string reason;
  const bool ok = risk.allow(intent, reason);

  std::cout << "[paper_trader] risk allow: " << (ok ? "true" : "false");
  if (!ok) std::cout << " reason=" << reason;
  std::cout << "\n";
  return 0;
}
