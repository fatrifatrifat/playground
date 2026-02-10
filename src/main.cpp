#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <expected>
#include <functional>
#include <list>
#include <map>
#include <mutex>
#include <print>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

namespace py = pybind11;

struct Pet {
  Pet(const std::string &name) : name(name) {}
  void setName(const std::string &name_) { name = name_; }
  const std::string &getName() const { return name; }

  std::string name;
};

struct Tick {
  double price;
  uint64_t volume;
  int64_t timestamp_ns;
};

struct Order {
  std::string symbol;
  double price;
  int quantity;
  bool is_buy;
};

struct BacktestEngine {
public:
  using StrategyCallback = std::move_only_function<void(const Tick &)>;

  std::expected<void, std::string> addTick(const Tick &t) noexcept {
    if (t.price <= 0)
      return std::unexpected("Invalid price");

    processTick(t);
    return {};
  }

  void setStrategy(StrategyCallback strategy) noexcept {
    strategy_ = std::move(strategy);
  }

private:
  void processTick(const Tick &t) {
    if (strategy_) {
      strategy_(t);
    }
  }

private:
  StrategyCallback strategy_;
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

  // Creaqting bindings for custom types
  py::class_<Pet>(m, "Pet")
      .def(py::init<const std::string &>())
      .def("setName", &Pet::setName)
      .def("getName", &Pet::getName)
      .def("__repr__",
           [](const Pet &a) { return "<my_app.Pet named '" + a.name + "'>"; })
      .def_readwrite("name", &Pet::name);

  py::class_<Tick>(m, "Tick")
      .def(py::init<double, uint64_t, int64_t>())
      .def_readwrite("price", &Tick::price)
      .def_readwrite("volume", &Tick::volume)
      .def_readwrite("timestamp_ns", &Tick::timestamp_ns);

  py::class_<BacktestEngine>(m, "Engine")
      .def(py::init<>())
      // Expose the tick ingestion
      .def("add_tick",
           [](BacktestEngine &self, Tick t) {
             auto res = self.addTick(t);
             if (!res)
               throw std::runtime_error(res.error());
           })
      // Allow Python users to define the strategy logic
      .def("set_strategy", &BacktestEngine::setStrategy);
}

int main() { return 0; }
