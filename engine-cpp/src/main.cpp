#include <iostream>
#include <trading/core/trading_engine.h>

static constexpr std::string PORT_HOST = "0.0.0.0:50051";

int main(int argc, char **argv) {
  quarcc::TradingEngine engine;
  if (argc != 2) {
    engine.Run(PORT_HOST.c_str());
    return 0;
  }

  engine.Run(argv[1]);
}
