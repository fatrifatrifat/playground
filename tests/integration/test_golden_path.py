"""
E2E smoke test: register => market data => order => fill => position => shutdown.

Requires a built engine binary (see conftest.py)
Runs against the paper gateway and simulated feed so no external connectivity is needed
"""

import os
import sys
import threading

_REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(_REPO_ROOT, "python_client"))
sys.path.insert(0, _REPO_ROOT)

from strategy.base_strategy import BaseStrategy
from strategy.config import MarketData, StrategyConfig, Subscription

_SERVER = "localhost:50051"
_SYMBOL = "ACDC"


def test_golden_path(engine_proc):
    """
    register => tick => BUY => fill => assert(position != 0) => shutdown
    """
    fills: list[dict] = []
    final_position: list[dict] = []
    done = threading.Event()

    class SmokeStrategy(BaseStrategy):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, **kwargs)
            self._order_placed = False

        def on_tick(self, tick):
            if not self._order_placed:
                order_id = self.buy(tick.symbol, qty=1.0)
                if order_id is not None:
                    self._order_placed = True

        def on_fill(self, fill):
            fills.append({
                "qty": fill.filled_quantity,
                "price": fill.avg_fill_price,
            })
            pos = self.get_position(_SYMBOL)
            if pos:
                final_position.append(pos)
            self.stop(reason="smoke test complete")

        def on_stop(self):
            done.set()

    config = StrategyConfig(
        strategy_id="smoke_test",
        account_id="test_acct",
        gateway=StrategyConfig.Gateway.PAPER_TRADING,
        market_data=MarketData(
            feed=MarketData.Feed.SIMULATED,
            subscriptions=[Subscription(_SYMBOL)],
        ),
    )

    strategy = SmokeStrategy(config, server_address=_SERVER)
    t = threading.Thread(target=strategy.run, daemon=True)
    t.start()

    assert done.wait(timeout=30), "strategy did not complete within 30 seconds"
    t.join(timeout=5)

    assert len(fills) >= 1, f"expected at least one fill, got {fills}"
    assert len(final_position) >= 1, "position was not recorded"
    assert final_position[0]["quantity"] != 0.0, (
        f"expected non-zero position, got {final_position[0]}"
    )
