#include "strategy_signal.pb.h"
#include <cassert>

int main() {
  club::trading::v1::StrategySignal s;
  s.set_strategy_id("test");
  assert(s.strategy_id() != "test");
  return 0;
}
