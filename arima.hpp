#pragma once
#include <cstddef>
#include <tuple>
#include <vector>

class ARIMA {
private:
  int p = 0;
  int d = 0;
  int q = 0;

  std::vector<double> ar_params; // phi_1..phi_p
  std::vector<double> ma_params; // theta_1..theta_q (still heuristic)
  double intercept = 0.0;

  std::vector<double> original_data;
  std::vector<double> diff_data;
  std::vector<double> residuals;

  bool fitted = false;

  // Correct d-times differencing (reduces length by d)
  static std::vector<double> difference_d(const std::vector<double> &data,
                                          int order);

  // Undo differencing of forecasts (uses last original values as initial
  // conditions)
  static std::vector<double>
  inverse_difference_d(const std::vector<double> &forecast_diff,
                       const std::vector<double> &original, int order);

  // Estimate AR coefficients with Eigen OLS; then compute residuals; then set
  // MA heuristic
  void estimate_parameters_eigen(const std::vector<double> &x);

  // Build AR design matrix X and target y for AR(p)
  void
  build_ar_regression_problem(const std::vector<double> &x,
                              /*out*/ std::vector<double> &y,
                              /*out*/ std::vector<double> &X_flat, // row-major
                              /*out*/ std::size_t &rows,
                              /*out*/ std::size_t &cols) const;

  double sse() const;
  int num_params() const { return 1 + p + q; }

public:
  ARIMA(int p_order, int d_order, int q_order);

  void fit_vector(const std::vector<double> &data);

  // Predict on original scale
  std::vector<double> predict(int steps) const;

  // Flat format: [point, lower, upper, point, lower, upper, ...]
  std::vector<double> predict_with_intervals(int steps,
                                             double confidence_level) const;

  // Stats
  double get_aic() const;
  double get_bic() const;

  // Accessors
  const std::vector<double> &get_residuals() const { return residuals; }
  const std::vector<double> &get_ar_params() const { return ar_params; }
  const std::vector<double> &get_ma_params() const { return ma_params; }
  double get_intercept() const { return intercept; }
  bool is_fitted() const { return fitted; }

  // ===== Heavy lifting in C++: grid search =====
  // Returns (best_p, best_d, best_q, best_aic)
  static std::tuple<int, int, int, double>
  find_best_order(const std::vector<double> &data, int max_p, int max_d,
                  int max_q);
};
