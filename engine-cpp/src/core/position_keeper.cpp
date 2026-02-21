#include <trading/core/position_keeper.h>

namespace quarcc {

Result<v1::Position>
PositionKeeper::getPosition(const std::string &symbol) const {
  std::shared_lock lock(mutex_);

  auto it = positions_.find(symbol);
  if (it == positions_.end()) {
    return std::unexpected(Error{"Position not found", ErrorType::Error});
  }

  v1::Position pos;
  pos.set_symbol(it->second.symbol);
  pos.set_quantity(it->second.quantity);
  pos.set_avg_price(it->second.avgPrice);

  return pos;
}

v1::PositionList PositionKeeper::getAllPositions() const {
  v1::PositionList all_pos;
  v1::Position pos;

  for (auto &[symbol, curr_pos] : positions_) {
    std::shared_lock lock(mutex_);

    pos.set_symbol(curr_pos.symbol);
    pos.set_quantity(curr_pos.quantity);
    pos.set_avg_price(curr_pos.avgPrice);
    all_pos.add_positions()->CopyFrom(pos);
  }

  return all_pos;
}

}; // namespace quarcc
