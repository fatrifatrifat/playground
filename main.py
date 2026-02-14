import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import quant_engine as qe
from statsmodels.graphics.tsaplots import plot_acf, plot_pacf
from statsmodels.stats.diagnostic import acorr_ljungbox
from statsmodels.tsa.arima.model import ARIMA
from statsmodels.tsa.stattools import adfuller

# # 1. Generate Fake Tick Data (Random Walk)
# def generate_fake_data(n=100_000):
#     returns = np.random.normal(0, 0.001, n)
#     price = 150.0 + np.cumsum(returns)
#     return price.astype(np.float64)
#
#
# # 2. Define the Strategy Logic (Python side)
# def execute_strategy(prices, signals):
#     # This happens AFTER the C++ engine crunched the numbers
#     buy_signals = np.where(signals > 0, 1, 0)
#     # Simple profit calculation
#     pnl = np.diff(prices) * buy_signals[:-1]
#     return np.cumsum(pnl)
#
#
# # 3. Execution
# engine = qe.Engine()
# prices = generate_fake_data(20_000_000)
#
# print("Running C++ Engine...")
# # C++ handles the 12-26-9 MACD logic in nanoseconds
# macd_histogram = engine.run_macd_backtest(prices, 12, 26, 9)
#
# print("Calculating PnL in Python...")
# pnl_curve = execute_strategy(prices, macd_histogram)
#
# plt.plot(pnl_curve)
# plt.title("Backtest Result: MACD 12-26-9")
# plt.show()


# ----------------------------
# 1) Load your quarterly series
# ----------------------------
# Replace this with your own data load.
# Expected format: a CSV with columns: date, sales
# Example date format: 2020-03-31, 2020-06-30, etc.

# df = pd.read_csv("quarterly_sales.csv", parse_dates=["date"])
# df = df.set_index("date").sort_index()
# y = df["sales"]

# Demo data (remove this block when using real data)
dates = pd.date_range("2019-03-31", periods=28, freq="Q")
rng = np.random.default_rng(0)
y = pd.Series(
    100
    + np.cumsum(rng.normal(2.0, 5.0, size=len(dates)))
    + 3 * np.sin(np.arange(len(dates)) / 2),
    index=dates,
)
y.name = "sales"

# ----------------------------
# 2) Optional: log transform (often good for Sales/EPS)
# ----------------------------
y_t = np.log(y)  # comment out if you don't want log

# ----------------------------
# 3) Visualize
# ----------------------------
y_t.plot(title="Transformed series (log(sales))")
plt.show()


# ----------------------------
# 4) Decide differencing d
# ----------------------------
def adf_pvalue(series):
    series = series.dropna()
    return adfuller(series, autolag="AIC")[1]


print("ADF p-value (d=0):", adf_pvalue(y_t))

# Try d = 1
y_diff = y_t.diff(1)
print("ADF p-value (d=1):", adf_pvalue(y_diff))

y_diff.plot(title="Differenced series (d=1)")
plt.show()

# ----------------------------
# 5) ACF/PACF to suggest p and q (use differenced series)
# ----------------------------
plot_acf(y_diff.dropna(), lags=12)
plt.show()

plot_pacf(y_diff.dropna(), lags=12, method="ywm")
plt.show()

# ----------------------------
# 6) Fit a few candidate models and compare AIC
# ----------------------------
candidates = []
for p in range(0, 4):
    for q in range(0, 4):
        try:
            model = ARIMA(y_t, order=(p, 1, q))  # d fixed to 1 here
            res = model.fit()
            candidates.append((p, 1, q, res.aic))
        except Exception:
            pass

candidates = sorted(candidates, key=lambda x: x[3])
print("Top models by AIC:")
for row in candidates[:5]:
    print(row)

best_p, best_d, best_q, best_aic = candidates[0]
print("Best by AIC:", (best_p, best_d, best_q), "AIC=", best_aic)

# ----------------------------
# 7) Fit best model and check diagnostics
# ----------------------------
best_model = ARIMA(y_t, order=(best_p, best_d, best_q))
best_res = best_model.fit()
print(best_res.summary())

# Residual checks
resid = best_res.resid.dropna()
plt.figure()
plt.plot(resid)
plt.title("Residuals")
plt.show()

plot_acf(resid, lags=12)
plt.show()

lb = acorr_ljungbox(resid, lags=[8, 12], return_df=True)
print("Ljung-Box test:")
print(lb)

# ----------------------------
# 8) Forecast next quarter
# ----------------------------
fc = best_res.get_forecast(steps=1)
mean = fc.predicted_mean.iloc[0]
ci = fc.conf_int(alpha=0.05).iloc[0]  # 95% interval

print("Forecast (transformed):", mean)
print("95% CI (transformed):", tuple(ci))

# If you used log, convert back to original scale:
mean_orig = float(np.exp(mean))
ci_orig = tuple(np.exp(ci))
print("Forecast (original scale):", mean_orig)
print("95% CI (original scale):", ci_orig)
