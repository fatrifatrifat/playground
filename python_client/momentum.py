from strategy.base_strategy import BaseStrategy
from strategy.config import MarketData, StrategyConfig, Subscription

tick_counter = 0
bar_counter = 0


class MomentumStrategy(BaseStrategy):
    def on_start(self):
        print("strategy started")

    def on_bar(self, bar):
        global bar_counter
        bar_counter += 1
        print(f"bar #{bar_counter}: {bar.symbol}, open: {bar.open}, close: {bar.close}")
        if bar.close > bar.open:
            self.buy(bar.symbol, qty=1.0)
        if bar_counter == 10:
            self.stop(reason="momentum strategy reached bar limit")
            return

    def on_tick(self, tick):
        global tick_counter
        # tick_counter += 1
        print(
            f"tick #{tick_counter}: {tick.symbol}, bid: {tick.bid}, ask: {tick.ask}, ts: {tick.timestamp_ns}"
        )
        while (True):
            self.buy(tick.symbol, qty=1.0)

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
            Subscription("C"),
        ],
    ),
)
MomentumStrategy(config).run()
