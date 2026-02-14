import datetime

import arima_forecast
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import pandas_datareader.data as web


class SalesForecaster:
    def __init__(self, data: pd.Series, dates: pd.Series = None):
        self.data = np.asarray(data, dtype=np.float64)  # numpy 1D
        self.dates = (
            dates
            if dates is not None
            else pd.date_range(start="2020-03-31", periods=len(self.data), freq="QE")
        )
        self.best_order = None
        self.best_aic = None
        self.model = None

    def auto_select_order(self, max_p=3, max_d=2, max_q=3):
        p, d, q, aic = arima_forecast.best_order(self.data, max_p, max_d, max_q)
        self.best_order = (int(p), int(d), int(q))
        self.best_aic = float(aic)
        print(f"Selected: ARIMA{self.best_order}, AIC={self.best_aic:.2f}")
        return self.best_order

    def fit(self, p=None, d=None, q=None):
        if p is None or d is None or q is None:
            if self.best_order is None:
                self.auto_select_order()
            p, d, q = self.best_order

        self.model = arima_forecast.ARIMA(int(p), int(d), int(q))
        self.model.fit(self.data)  # numpy array goes in, no tolist
        print("Fitted.")
        print("AR:", self.model.get_ar_params())
        print("MA:", self.model.get_ma_params())
        print("Intercept:", self.model.get_intercept())
        print("AIC:", self.model.get_aic())
        print("BIC:", self.model.get_bic())

    def forecast(self, steps=4, confidence_level=0.95):
        if self.model is None:
            raise ValueError("Call fit() first")

        flat = self.model.predict_with_intervals(int(steps), float(confidence_level))
        flat = np.asarray(flat, dtype=np.float64).reshape(steps, 3)

        last_date = self.dates.iloc[-1]
        future_dates = pd.date_range(
            start=last_date + pd.DateOffset(months=3), periods=steps, freq="QE"
        )

        df = pd.DataFrame(
            flat, index=future_dates, columns=["Forecast", "Lower_Bound", "Upper_Bound"]
        )
        return df

    def plot_forecast(self, forecast_df):
        plt.figure(figsize=(12, 6))
        plt.plot(self.dates, self.data, "o-", label="Historical", linewidth=2)
        plt.plot(
            forecast_df.index,
            forecast_df["Forecast"],
            "s--",
            label="Forecast",
            linewidth=2,
        )
        plt.fill_between(
            forecast_df.index,
            forecast_df["Lower_Bound"],
            forecast_df["Upper_Bound"],
            alpha=0.25,
            label="Interval",
        )
        plt.grid(True, alpha=0.3)
        plt.legend()
        plt.xticks(rotation=45)
        plt.tight_layout()
        return plt.gcf()


def main():
    """Example usage with sample quarterly sales data (C++ does best-order search)"""

    print("=" * 70)
    print("ARIMA Sales Forecasting System")
    print("C++ Backend (Eigen + C++ grid search) with Python Interface")
    print("=" * 70)

    start = datetime.datetime(2015, 1, 1)
    end = datetime.datetime(2024, 1, 1)
    
    df = web.DataReader("RSAFS", "fred", start, end).dropna()
    
    sales = df["RSAFS"]
    dates = sales.index
    
    forecaster = SalesForecaster(sales, dates)

    print("\nHistorical Sales Data:")
    print(df)

    # Initialize forecaster (data stored as numpy float64 internally)
    forecaster = SalesForecaster(df["Sales"], df["Quarter"])

    # C++ grid search (single call): returns best (p,d,q) + best AIC
    forecaster.auto_select_order(max_p=3, max_d=2, max_q=3)

    # Fit chosen model (C++ Eigen OLS)
    forecaster.fit()

    # Forecast next 4 quarters (C++ intervals)
    forecast_df = forecaster.forecast(steps=4, confidence_level=0.95)

    print("\nForecast Results:")
    print(forecast_df)

    # Lightweight metrics (from C++ stats + residuals)
    print("\nModel Stats:")
    print(f"  AIC: {forecaster.model.get_aic():.2f}")
    print(f"  BIC: {forecaster.model.get_bic():.2f}")

    # If you want RMSE/MAE like before (in-sample):
    resid = np.asarray(forecaster.model.get_residuals(), dtype=np.float64)
    if resid.size > 0:
        rmse = float(np.sqrt(np.mean(resid**2)))
        mae = float(np.mean(np.abs(resid)))
        print(f"  RMSE (in-sample): {rmse:.2f}")
        print(f"  MAE  (in-sample): {mae:.2f}")

    # Visualizations
    print("\nGenerating visualizations...")

    fig1 = forecaster.plot_forecast(forecast_df)
    plt.savefig("sales_forecast.png", dpi=300, bbox_inches="tight")
    print("  Saved: sales_forecast.png")

    # Optional: if you kept/ported plot_diagnostics() in your class, you can enable this.
    if hasattr(forecaster, "plot_diagnostics"):
        fig2 = forecaster.plot_diagnostics()
        plt.savefig("model_diagnostics.png", dpi=300, bbox_inches="tight")
        print("  Saved: model_diagnostics.png")

    plt.close("all")

    print("\n" + "=" * 70)
    print("Forecasting complete!")
    print("=" * 70)

    return forecaster, forecast_df


if __name__ == "__main__":
    forecaster, forecast = main()
