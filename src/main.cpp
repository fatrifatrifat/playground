#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <mutex>
#include <print>
#include <pybind11/pybind11.h>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

namespace py = pybind11;

#define MAX_PRODUCE 20
#define MAX_CONSUME 20

struct SafeQueue {
public:
  void push(int v) {
    std::unique_lock lk{mtx};
    cv.wait(lk, [&] { return q.size() < cap; });
    q.push(v);
    cv.notify_one();
  }

  int pop() {
    std::unique_lock lk{mtx};
    cv.wait(lk, [&] { return !q.empty(); });
    int temp = q.front();
    q.pop();
    cv.notify_one();
    return temp;
  }

private:
  std::mutex mtx;
  std::condition_variable cv;

  std::queue<int> q;
  const std::size_t cap = 10;
};

using Id = size_t;
using Price = long;
using Quantity = int;

class Order {
public:
  Order(Id orderId, Price level, bool isBuy, Quantity quantity)
      : id_(orderId), level_(level), isBuy_(isBuy), quantity_(quantity) {}

  Id id() const { return id_; }
  Price level() const { return level_; }
  bool isBuy() const { return isBuy_; }
  Quantity quantity() const { return quantity_; }

private:
  Id id_;
  Price level_;
  bool isBuy_;
  Quantity quantity_;
};

using Orders = std::vector<Order>;

struct Trade {
  Id OrderIdA;
  Id OrderIdB;
  Id AggressorOrderId;
  bool AggressorIsBuy;
  Price Level;
  Quantity Size;
};

using Trades = std::vector<Trade>;

class Orderbook {
public:
  Trades AddOrder(const Order &order) {
    Trades trades;

    if (orderById_.contains(order.id())) {
      return trades;
    }

    if (order.isBuy()) {
      auto &fifoList = bids_[order.level()];
      fifoList.push_back(order.id());
      auto it = std::prev(fifoList.end());

      const OrderState st{
          .id_ = order.id(),
          .level_ = order.level(),
          .isBuy_ = order.isBuy(),
          .remaining_ = order.quantity(),
          .iterator_ = it,
      };

      orderById_.emplace(order.id(), st);

      return Match(order.id());
    } else {
      auto &fifoList = asks_[order.level()];
      fifoList.push_back(order.id());
      auto it = std::prev(fifoList.end());

      const OrderState st{
          .id_ = order.id(),
          .level_ = order.level(),
          .isBuy_ = order.isBuy(),
          .remaining_ = order.quantity(),
          .iterator_ = it,
      };

      orderById_.emplace(order.id(), st);

      return Match(order.id());
    }

    return {};
  }

  void CancelOrder(Id orderId) {
    auto it = orderById_.find(orderId);
    if (it == orderById_.end()) {
      return;
    }

    const OrderState &state = it->second;
    if (state.isBuy_) {
      auto bidsIt = bids_.find(state.level_);
      if (bidsIt == bids_.end()) {
        return;
      }
      bidsIt->second.erase(state.iterator_);
    } else {
      auto asksIt = asks_.find(state.level_);
      if (asksIt == asks_.end()) {
        return;
      }
      asksIt->second.erase(state.iterator_);
    }
  }

private:
  struct OrderState {
    Id id_;
    Price level_;
    bool isBuy_;
    Quantity remaining_;
    std::list<Id>::iterator iterator_;
  };

  bool processOrder(bool isAggressorBuy, Price aggressorPrice,
                    Price restingPrice) const noexcept {
    return isAggressorBuy ? (aggressorPrice >= restingPrice)
                          : (aggressorPrice <= restingPrice);
  }

  Trades Match(Id aggressorId) {
    Trades trades;

    auto aggressorIt = orderById_.find(aggressorId);
    if (aggressorIt == orderById_.end()) {
      return {};
    }

    auto &aggressor = aggressorIt->second;
    while (aggressor.remaining_ > 0) {
      if (aggressor.isBuy_) {
        if (asks_.empty()) {
          return {};
        }

        auto askFifoIt = asks_.begin();
        if (!processOrder(true, aggressor.level_, askFifoIt->first)) {
          return {};
        }

        Id restingId = askFifoIt->second.front();
        auto restIt = orderById_.find(restingId);
        if (restIt == orderById_.end()) {
          askFifoIt->second.pop_front();
          if (askFifoIt->second.empty())
            asks_.erase(askFifoIt);
          continue;
        }

        OrderState &resting = restIt->second;
        Quantity execQty = std::min(aggressor.remaining_, resting.remaining_);

        trades.push_back(Trade{.OrderIdA = resting.id_,
                               .OrderIdB = aggressor.id_,
                               .AggressorOrderId = aggressor.id_,
                               .AggressorIsBuy = aggressor.isBuy_,
                               .Level = resting.level_,
                               .Size = execQty});

        aggressor.remaining_ -= execQty;
        resting.remaining_ -= execQty;

        if (aggressor.remaining_ == 0) {
          auto bidsIt = bids_.find(aggressor.level_);
          if (bidsIt != bids_.end()) {
            bidsIt->second.erase(aggressor.iterator_);
          }
          orderById_.erase(aggressorIt);
        }
        if (resting.remaining_ == 0) {
          auto asksIt = asks_.find(resting.level_);
          if (asksIt != asks_.end()) {
            asksIt->second.erase(resting.iterator_);
          }
          orderById_.erase(restIt);
        }
      } else {
        if (bids_.empty()) {
          return {};
        }

        auto bidsFifoIt = bids_.begin();
        if (!processOrder(true, aggressor.level_, bidsFifoIt->first)) {
          return {};
        }

        Id restingId = bidsFifoIt->second.front();
        auto restIt = orderById_.find(restingId);
        if (restIt == orderById_.end()) {
          bidsFifoIt->second.pop_front();
          if (bidsFifoIt->second.empty())
            bids_.erase(bidsFifoIt);
          continue;
        }

        OrderState &resting = restIt->second;
        Quantity execQty = std::min(aggressor.remaining_, resting.remaining_);

        trades.push_back(Trade{.OrderIdA = resting.id_,
                               .OrderIdB = aggressor.id_,
                               .AggressorOrderId = aggressor.id_,
                               .AggressorIsBuy = aggressor.isBuy_,
                               .Level = resting.level_,
                               .Size = execQty});

        aggressor.remaining_ -= execQty;
        resting.remaining_ -= execQty;

        if (aggressor.remaining_ == 0) {
          auto asksIt = asks_.find(aggressor.level_);
          if (asksIt != asks_.end()) {
            asksIt->second.erase(aggressor.iterator_);
          }
          orderById_.erase(aggressorIt);
        }
        if (resting.remaining_ == 0) {
          auto bidsIt = bids_.find(resting.level_);
          if (bidsIt != bids_.end()) {
            bidsIt->second.erase(resting.iterator_);
          }
          orderById_.erase(restIt);
        }
      }
    }
    return trades;
  }

private:
  std::map<Price, std::list<Id>, std::greater<Price>> bids_;
  std::map<Price, std::list<Id>> asks_;

  std::unordered_map<Id, OrderState> orderById_;
};

int add(int i, int j) { return i + j; }

PYBIND11_MODULE(my_app, m, py::mod_gil_not_used()) {
  m.doc() = "pybind11 my_app plugin";

  // Specify arguments with py::arg
  m.def("add0", &add, "A function that adds two numbers", py::arg("i"),
        py::arg("j"));

  // Specify arguments with pybind11::literals::_a
  using namespace pybind11::literals;
  m.def("add1", &add, "A function that adds two numbers", "i"_a, "j"_a);

  // Specify default arguments
  m.def("add2", &add, "A function that adds two numbers", "i"_a = 1, "j"_a = 2);

  // Exporting variables
  m.attr("the_answer") = 42;
  py::object world = py::cast("World");
  m.attr("what") = world;
}

int main() { return 0; }
