#include <cstddef>
#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <vector>

namespace py = pybind11;

class BacktestEngine {
public:
  // Optimized EMA calculation (Incremental)
  double calculate_ema(double current_price, double prev_ema, int period) {
    double k = 2.0 / (period + 1.0);
    return (current_price * k) + (prev_ema * (1.0 - k));
  }

  // Process a whole batch of ticks at once
  py::array_t<double> run_macd_backtest(py::array_t<double> prices, int fast_p,
                                        int slow_p, int signal_p) {
    const auto buf = prices.request();
    double *ptr = static_cast<double *>(buf.ptr);
    size_t size = buf.shape[0];

    // Output buffer for MACD Histogram
    auto result = py::array_t<double>(size);
    double *res_ptr = static_cast<double *>(result.request().ptr);

    double ema_fast = ptr[0];
    double ema_slow = ptr[0];
    double ema_signal = 0;

    for (size_t i = 0; i < size; ++i) {
      ema_fast = calculate_ema(ptr[i], ema_fast, fast_p);
      ema_slow = calculate_ema(ptr[i], ema_slow, slow_p);

      double macd_line = ema_fast - ema_slow;
      ema_signal =
          (i == 0) ? macd_line : calculate_ema(macd_line, ema_signal, signal_p);

      // The "Histogram" is the signal we usually trade on
      res_ptr[i] = macd_line - ema_signal;
    }

    return result;
  }
};

PYBIND11_MODULE(quant_engine, m) {
  py::class_<BacktestEngine>(m, "Engine")
      .def(py::init<>())
      .def("run_macd_backtest", &BacktestEngine::run_macd_backtest);
}

int main() { return 0; }
