#pragma once

#include <trading/core/order_manager.h>

#include <optional>
#include <string>
#include <vector>

namespace quarcc {

struct MarketDataSubscription {
  Symbol symbol;
  std::optional<BarPeriod> period = std::nullopt; // e.g. "1m", "5m", "1d"
};

struct MarketDataConfig {
  std::string feed; // "alpaca", "csv", "simulated"
  std::vector<MarketDataSubscription> subscriptions;
};

// Only when gateway == GRPC_ADAPTER in the user's config
struct AdapterConfig {
  static inline const std::string binary_path =
      "python_client/adapters/adapter.py"; // path to the Python adapter script
                                           // has to be ran through the project
                                           // root
  std::string venue; // "ibkr", "binance", "polymarket", "paper trading", etc.
                     // (even tho none of them are implemented yet)
  std::string credentials_path; // path to credentials for this venue
  int port{0};                  // port the adapter listens on (must be unique
                                // per (venue, account_id) on this host)
};

struct StrategyConfig {
  std::string id;
  std::string account_id;
  std::string gateway;
  std::optional<AdapterConfig> adapter; // required when gateway == GRPC_ADAPTER
  // Optional: not all strategies need market data from the engine
  std::optional<MarketDataConfig> market_data;
};

} // namespace quarcc
