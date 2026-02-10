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

struct Pet {
  Pet(const std::string &name) : name(name) {}
  void setName(const std::string &name_) { name = name_; }
  const std::string &getName() const { return name; }

  std::string name;
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
}

int main() { return 0; }
