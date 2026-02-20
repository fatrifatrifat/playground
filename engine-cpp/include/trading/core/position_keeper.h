#pragma once

#include "execution_service.pb.h"
#include "order.pb.h"

#include <trading/utils/order_id_generator.h>
#include <trading/utils/result.h>

#include <condition_variable>
#include <mutex>
#include <unordered_set>

namespace quarcc {

class PositionKeeper {
public:
  void newOrderAdded(const v1::Order &new_order);

  std::mutex &getMutex() noexcept;
  std::condition_variable &getConditionVariable() noexcept;

private:
  void fetchPosition();

private:
  // TODO: OrderId or Symbol? For now: OrderId
  std::unordered_set<OrderId> orders_to_check_;
  v1::PositionList position_list_;
  std::condition_variable cv_;
  std::mutex mu_;

  std::uint32_t new_order_count{};
};

}; // namespace quarcc
