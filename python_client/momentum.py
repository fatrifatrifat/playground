from strategy.base_strategy import BaseStrategy
from strategy.config import MarketData, StrategyConfig, Subscription

counter = 0


class MomentumStrategy(BaseStrategy):
    def on_start(self):
        print("strategy started")

    def on_bar(self, bar):
        global counter
        print(f"bar #{counter}: {bar.symbol}, open: {bar.open}, close: {bar.close}")
        if bar.close > bar.open:
            self.buy(bar.symbol, qty=1.0)
        if counter == 10:
            self.stop(reason="momentum strategy reached bar limit")
            return
        counter += 1

    def on_tick(self, tick):
        global counter
        print(
            f"tick #{counter}: {tick.symbol}, bid: {tick.bid}, ask: {tick.ask}, ts: {tick.timestamp_ns}"
        )
        if tick.bid > 105:
            self.buy(tick.symbol, qty=1.0)
        counter += 1

    def on_fill(self, fill):
        print(f"fill: {fill.filled_quantity} @ {fill.avg_fill_price}")

    def on_stop(self):
        print("strategy stopped")


config = StrategyConfig(
    strategy_id="momentum_1",
    account_id="acct_001",
    gateway=StrategyConfig.Gateway.PAPER_TRADING,
    market_data=MarketData(
        MarketData.Feed.SIMULATED,
        [
            Subscription("AAPL", "1m"),
            Subscription("A", "1s"),
            Subscription("B", "5m"),
            Subscription("C"),
        ],
    ),
)
MomentumStrategy(config).run()
