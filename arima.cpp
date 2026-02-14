#include "arima.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>

// Eigen
#include <Eigen/Dense>

ARIMA::ARIMA(int p_order, int d_order, int q_order)
    : p(p_order), d(d_order), q(q_order) {
  if (p < 0 || d < 0 || q < 0)
    throw std::invalid_argument("Orders must be non-negative");
  ar_params.assign(p, 0.0);
  ma_params.assign(q, 0.0);
}

std::vector<double> ARIMA::difference_d(const std::vector<double> &data,
                                        int order) {
  if (order == 0)
    return data;
  if ((int)data.size() <= order)
    throw std::runtime_error("Not enough points for differencing");

  std::vector<double> cur = data;
  for (int k = 0; k < order; ++k) {
    std::vector<double> nxt;
    nxt.reserve(cur.size() - 1);
    for (std::size_t i = 1; i < cur.size(); ++i) {
      nxt.push_back(cur[i] - cur[i - 1]);
    }
    cur = std::move(nxt);
  }
  return cur;
}

std::vector<double>
ARIMA::inverse_difference_d(const std::vector<double> &forecast_diff,
                            const std::vector<double> &original, int order) {
  if (order == 0)
    return forecast_diff;
  if ((int)original.size() < order)
    throw std::runtime_error("Not enough original data to invert differencing");

  // We reconstruct by repeatedly "integrating" forecasts.
  // For d=1: y_{T+h} = y_T + sum_{i=1..h} x_{T+i}
  // For d>1: integrate multiple times using last observed values as initial
  // conditions.

  std::vector<double> cur = forecast_diff;

  // We need initial conditions: last values of each differencing level.
  // We'll compute the last values of Δ^k y for k=0..order-1 from original.
  std::vector<std::vector<double>> levels;
  levels.reserve(order);
  levels.push_back(original); // level 0 is original y

  for (int k = 1; k < order; ++k) {
    levels.push_back(difference_d(levels.back(), 1)); // one step difference
  }

  // last value of Δ^k y
  std::vector<double> last(order);
  for (int k = 0; k < order; ++k) {
    last[k] = levels[k].back();
  }

  // Now integrate from highest difference down to level 0.
  // We'll build Δ^{order-1} series forecasts, then integrate step-by-step to y.
  // Start with Δ^order forecasts = cur (what model predicts on differenced
  // scale). Integrate once -> Δ^{order-1} forecasted values, etc.
  for (int k = order - 1; k >= 0; --k) {
    // Integrate cur into level k values using last[k] as starting point.
    std::vector<double> out;
    out.reserve(cur.size());
    double prev = last[k];
    for (double dx : cur) {
      double val = prev + dx;
      out.push_back(val);
      prev = val;
    }
    cur = std::move(out);
    // After first integration, next loop integrates again, etc.
  }
  // After integrating order times, cur is forecast on original level.
  return cur;
}

void ARIMA::build_ar_regression_problem(const std::vector<double> &x,
                                        std::vector<double> &y,
                                        std::vector<double> &X_flat,
                                        std::size_t &rows,
                                        std::size_t &cols) const {
  // For AR(p): x_t = c + sum_{i=1..p} phi_i x_{t-i} + e_t
  // We create rows for t = p..n-1
  if (p == 0)
    throw std::runtime_error("AR regression requested with p=0");

  const std::size_t n = x.size();
  if (n <= (std::size_t)p)
    throw std::runtime_error("Not enough points for AR regression");

  rows = n - (std::size_t)p;
  cols = (std::size_t)p + 1; // intercept + p lags

  y.assign(rows, 0.0);
  X_flat.assign(rows * cols, 0.0);

  // Row r corresponds to time t = p + r
  for (std::size_t r = 0; r < rows; ++r) {
    const std::size_t t = (std::size_t)p + r;
    y[r] = x[t];

    // intercept
    X_flat[r * cols + 0] = 1.0;

    // lags: x[t-1], x[t-2], ..., x[t-p]
    for (int lag = 1; lag <= p; ++lag) {
      X_flat[r * cols + (std::size_t)lag] = x[t - (std::size_t)lag];
    }
  }
}

void ARIMA::estimate_parameters_eigen(const std::vector<double> &x) {
  if (x.size() < 10)
    throw std::runtime_error(
        "Need at least 10 points after differencing for stable fit");

  // Reset
  intercept = 0.0;
  std::fill(ar_params.begin(), ar_params.end(), 0.0);
  std::fill(ma_params.begin(), ma_params.end(), 0.0);
  residuals.clear();

  if (p == 0) {
    // Mean-only model
    intercept = std::accumulate(x.begin(), x.end(), 0.0) / (double)x.size();
    residuals.reserve(x.size());
    for (double v : x)
      residuals.push_back(v - intercept);
  } else {
    // Build regression problem
    std::vector<double> y_vec;
    std::vector<double> X_flat;
    std::size_t rows = 0, cols = 0;
    build_ar_regression_problem(x, y_vec, X_flat, rows, cols);

    // Map into Eigen without extra copies (Eigen will reference our buffers)
    Eigen::Map<const Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic,
                                   Eigen::RowMajor>>
        X(X_flat.data(), (int)rows, (int)cols);
    Eigen::Map<const Eigen::VectorXd> y(y_vec.data(), (int)rows);

    // Solve OLS with QR (more stable than normal equations)
    Eigen::VectorXd beta = X.colPivHouseholderQr().solve(y);

    intercept = beta[0];
    for (int i = 0; i < p; ++i) {
      ar_params[i] = beta[i + 1];
    }

    // Compute residuals for the rows used in regression
    Eigen::VectorXd y_hat = X * beta;
    residuals.resize(rows);
    for (std::size_t i = 0; i < rows; ++i) {
      residuals[i] = y_vec[i] - y_hat[(int)i];
    }
  }

  // MA heuristic (still simplified): set theta_k based on residual
  // autocovariance
  if (q > 0) {
    const int R = (int)residuals.size();
    for (int k = 1; k <= q; ++k) {
      if (R <= k) {
        ma_params[k - 1] = 0.0;
        continue;
      }

      double sum = 0.0;
      int count = 0;
      for (int t = k; t < R; ++t) {
        sum += residuals[t] * residuals[t - k];
        ++count;
      }

      // Normalize by variance to keep scale reasonable (still not “true MA
      // MLE”)
      double var = 0.0;
      for (double e : residuals)
        var += e * e;
      var /= std::max(1, R);

      double acf = (var > 1e-12) ? (sum / (double)count) / var : 0.0;
      ma_params[k - 1] = -acf; // heuristic sign
    }
  }
}

void ARIMA::fit_vector(const std::vector<double> &data) {
  if (data.size() < 10)
    throw std::invalid_argument("Need at least 10 data points for fitting");

  original_data = data;
  diff_data = difference_d(original_data, d);
  estimate_parameters_eigen(diff_data);
  fitted = true;
}

std::vector<double> ARIMA::predict(int steps) const {
  if (!fitted)
    throw std::runtime_error("Model must be fitted before prediction");
  if (steps <= 0)
    throw std::invalid_argument("Steps must be positive");

  // Predict on differenced scale first
  std::vector<double> ext_x = diff_data;

  // We store residuals aligned to regression rows. For forecasting, keep last
  // residuals.
  std::vector<double> ext_e = residuals;
  if (ext_e.empty())
    ext_e.push_back(0.0);

  std::vector<double> forecast_diff;
  forecast_diff.reserve(steps);

  for (int h = 0; h < steps; ++h) {
    double pred = intercept;

    // AR: use last p values of ext_x
    for (int i = 0; i < p; ++i) {
      const std::size_t idx = ext_x.size() - 1 - (std::size_t)i;
      pred += ar_params[i] * ext_x[idx];
    }

    // MA: use last q residuals; future residual assumed 0 after stepping
    for (int i = 0; i < q; ++i) {
      const std::size_t idx = ext_e.size() - 1 - (std::size_t)i;
      pred += ma_params[i] * ext_e[idx];
    }

    forecast_diff.push_back(pred);
    ext_x.push_back(pred);
    ext_e.push_back(0.0); // assume 0 future innovation
  }

  // Bring back to original scale
  return inverse_difference_d(forecast_diff, original_data, d);
}

double ARIMA::sse() const {
  double sum = 0.0;
  for (double e : residuals)
    sum += e * e;
  return sum;
}

double ARIMA::get_aic() const {
  if (!fitted)
    return std::numeric_limits<double>::infinity();
  const int n = (int)residuals.size();
  if (n <= 0)
    return std::numeric_limits<double>::infinity();

  const double SSE = std::max(1e-12, sse());
  const int k = num_params();

  // Gaussian errors: AIC ~ n*log(SSE/n) + 2k
  return (double)n * std::log(SSE / (double)n) + 2.0 * (double)k;
}

double ARIMA::get_bic() const {
  if (!fitted)
    return std::numeric_limits<double>::infinity();
  const int n = (int)residuals.size();
  if (n <= 0)
    return std::numeric_limits<double>::infinity();

  const double SSE = std::max(1e-12, sse());
  const int k = num_params();

  // BIC ~ n*log(SSE/n) + k*log(n)
  return (double)n * std::log(SSE / (double)n) +
         (double)k * std::log((double)n);
}

std::vector<double>
ARIMA::predict_with_intervals(int steps, double confidence_level) const {
  auto point = predict(steps);

  // residual standard deviation on the model scale
  const int n = (int)residuals.size();
  double mse = (n > 0) ? sse() / (double)n : 0.0;
  double sigma = std::sqrt(std::max(1e-12, mse));

  // z-score approximation
  double z = 1.96; // 95%
  if (std::abs(confidence_level - 0.90) < 0.01)
    z = 1.645;
  if (std::abs(confidence_level - 0.99) < 0.01)
    z = 2.576;

  // Simple horizon growth: std ~ sigma * sqrt(h)
  std::vector<double> out;
  out.reserve((std::size_t)steps * 3);

  for (int h = 1; h <= steps; ++h) {
    double se_h = sigma * std::sqrt((double)h);
    double lo = point[h - 1] - z * se_h;
    double hi = point[h - 1] + z * se_h;

    out.push_back(point[h - 1]);
    out.push_back(lo);
    out.push_back(hi);
  }
  return out;
}

std::tuple<int, int, int, double>
ARIMA::find_best_order(const std::vector<double> &data, int max_p, int max_d,
                       int max_q) {
  if (data.size() < 10)
    throw std::invalid_argument("Need at least 10 data points");

  double best_aic = std::numeric_limits<double>::infinity();
  int best_p = 1, best_d = 0, best_q = 0;

  // One C++ call does everything: no Python loop overhead.
  for (int pp = 0; pp <= max_p; ++pp) {
    for (int dd = 0; dd <= max_d; ++dd) {
      for (int qq = 0; qq <= max_q; ++qq) {
        if (pp == 0 && qq == 0)
          continue; // skip trivial

        try {
          ARIMA model(pp, dd, qq);
          model.fit_vector(data);
          double aic = model.get_aic();

          if (aic < best_aic) {
            best_aic = aic;
            best_p = pp;
            best_d = dd;
            best_q = qq;
          }
        } catch (...) {
          // skip unstable / invalid configurations
          continue;
        }
      }
    }
  }

  return {best_p, best_d, best_q, best_aic};
}
