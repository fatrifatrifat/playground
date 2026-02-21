#pragma once

#include "execution_service.pb.h"

#include <trading/utils/order_id_generator.h>
#include <trading/utils/result.h>

#include <shared_mutex>

namespace quarcc {

class PositionKeeper {
public:
  Result<v1::Position> getPosition(const std::string &symbol) const;
  v1::PositionList getAllPositions() const;

private:
  struct Position {
    std::string symbol;
    double quantity = 0.0;
    double avgPrice = 0.0;
  };

  mutable std::shared_mutex mutex_;
  std::unordered_map<std::string, Position> positions_;
};

}; // namespace quarcc
