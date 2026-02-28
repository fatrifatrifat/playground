#include <trading/core/position_keeper.h>

namespace quarcc {

// Applies a broker fill to the in-memory position using signed quantity and
// a weighted-average entry price.
//
// Signed-qty convention: long > 0, short < 0.
//
// Avg-price rules:
//  - Opening from flat: avg = fill_price
//  - Adding to existing side: weighted average
//  - Reducing (but not flipping): avg unchanged
//  - Position flips sides: avg = fill_price of new side
//  - Goes flat: avg = 0
//  - fill_price == 0 (gateway didn't provide it): update qty only
void PositionKeeper::on_fill(const std::string &symbol, double fill_qty,
                             double fill_price, v1::Side side) {
  if (fill_qty <= 0.0)
    return;

  std::unique_lock lock(mutex_);
  auto &pos = positions_[symbol];
  pos.symbol = symbol;

  const double signed_fill = (side == v1::Side::BUY) ? fill_qty : -fill_qty;
  const double old_qty = pos.quantity;
  const double new_qty = old_qty + signed_fill;

  if (fill_price > 0.0) {
    if (new_qty == 0.0) {
      // Position went flat
      pos.avgPrice = 0.0;
    } else if (old_qty == 0.0) {
      // Opening a brand-new position
      pos.avgPrice = fill_price;
    } else if ((old_qty > 0.0 && new_qty < 0.0) ||
               (old_qty < 0.0 && new_qty > 0.0)) {
      // Position flipped sides; the new position's cost basis is the fill
      pos.avgPrice = fill_price;
    } else if ((old_qty > 0.0 && signed_fill > 0.0) ||
               (old_qty < 0.0 && signed_fill < 0.0)) {
      // Adding to the existing side: weighted average
      pos.avgPrice =
          (old_qty * pos.avgPrice + signed_fill * fill_price) / new_qty;
    }
    // else: reducing position without flipping — avg price is unchanged
  }

  pos.quantity = new_qty;
}

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
  std::shared_lock lock(mutex_);

  v1::PositionList all_pos;
  for (const auto &[symbol, curr_pos] : positions_) {
    v1::Position pos;
    pos.set_symbol(curr_pos.symbol);
    pos.set_quantity(curr_pos.quantity);
    pos.set_avg_price(curr_pos.avgPrice);
    all_pos.add_positions()->CopyFrom(pos);
  }

  return all_pos;
}

}; // namespace quarcc
