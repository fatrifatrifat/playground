#include <trading/core/position_keeper.h>

namespace quarcc {

// Whenever OrderManager gets a new order, after passing through validation
// (risk manager, order book, etc.), tell position keeper to keep an eye on
// notifications through websockets for order status updating (accepted,
// pending, declined, etc.) to update position_list
void PositionKeeper::newOrderAdded(const v1::Order &new_order) {
  std::lock_guard lk(mu_);

  orders_to_check_.insert(new_order.id());

  if (orders_to_check_.empty()) {
    cv_.notify_one();
  }
}

void PositionKeeper::fetchPosition() {
  // TODO: Store order per symbol/order_id in a set, insert when new order comes
  // in, start fetching for positions with websocket, when broker notifies it's
  // filled, update position_list & remove from set

  std::unique_lock lk(mu_);
  while (new_order_count <= 0) {
    cv_.wait(lk);
  }

  // TODO: Getting status update on order
}

// Might not be needed, especially now that newOrderAdded() exists, to see
std::mutex &PositionKeeper::getMutex() noexcept { return mu_; }
std::condition_variable &PositionKeeper::getConditionVariable() noexcept {
  return cv_;
}

}; // namespace quarcc
