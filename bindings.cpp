#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "arima.hpp"

namespace py = pybind11;

static std::vector<double> numpy_to_vector_1d(
    py::array_t<double, py::array::c_style | py::array::forcecast> arr) {
  auto buf = arr.request();
  if (buf.ndim != 1)
    throw std::runtime_error("Expected 1D numpy array");

  const auto *ptr = static_cast<const double *>(buf.ptr);
  return std::vector<double>(ptr, ptr + buf.shape[0]);
}

PYBIND11_MODULE(arima_forecast, m) {
  m.doc() = "ARIMA forecasting (C++ heavy lifting: grid search + Eigen OLS)";

  m.def(
      "best_order",
      [](py::array_t<double, py::array::c_style | py::array::forcecast> data,
         int max_p, int max_d, int max_q) {
        auto v = numpy_to_vector_1d(data);
        auto [bp, bd, bq, baic] =
            ARIMA::find_best_order(v, max_p, max_d, max_q);
        return py::make_tuple(bp, bd, bq, baic);
      },
      py::arg("data"), py::arg("max_p") = 3, py::arg("max_d") = 2,
      py::arg("max_q") = 3);

  py::class_<ARIMA>(m, "ARIMA")
      .def(py::init<int, int, int>(), py::arg("p"), py::arg("d"), py::arg("q"))

      .def(
          "fit",
          [](ARIMA &self,
             py::array_t<double, py::array::c_style | py::array::forcecast>
                 data) { self.fit_vector(numpy_to_vector_1d(data)); },
          py::arg("data"))

      .def("predict", &ARIMA::predict, py::arg("steps"))
      .def("predict_with_intervals", &ARIMA::predict_with_intervals,
           py::arg("steps"), py::arg("confidence_level") = 0.95)

      .def("get_aic", &ARIMA::get_aic)
      .def("get_bic", &ARIMA::get_bic)
      .def("get_residuals", &ARIMA::get_residuals)
      .def("get_ar_params", &ARIMA::get_ar_params)
      .def("get_ma_params", &ARIMA::get_ma_params)
      .def("get_intercept", &ARIMA::get_intercept)
      .def("is_fitted", &ARIMA::is_fitted);
}
