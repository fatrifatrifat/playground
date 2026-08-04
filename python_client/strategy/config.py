from __future__ import annotations

import os
import sys
from dataclasses import dataclass, field
from enum import StrEnum
from typing import Optional

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../.."))


@dataclass
class Subscription:
    symbol: str
    period: Optional[str] = None


@dataclass
class MarketData:
    class Feed(StrEnum):
        SIMULATED = "simulated"
        ALPACA = "alpaca"
        ADAPTER = "adapter"
        # IBKR = "ibkr"

    feed: Feed
    subscriptions: list[Subscription] = field(default_factory=list)


@dataclass
class AdapterConfig:
    """Only needed when gateway == "grpc_adapter"."""

    venue: str
    credentials_path: str
    port: int


@dataclass
class RiskConfig:
    """Parameters for risk management"""

    max_quantity: float = 0.0
    max_open_orders: int = 0
    daily_pnl_limit: float = 0.0


@dataclass
class StrategyConfig:
    class Gateway(StrEnum):
        GRPC_ADAPTER = "grpc_adapter"
        PAPER_TRADING = "paper_trading"
        # BACKTESTING = "backtest"

    strategy_id: str
    account_id: str
    # PaperTrading, Alpaca, or GrpcAdapter
    gateway: Gateway
    market_data: Optional[MarketData] = None
    adapter: Optional[AdapterConfig] = None
    risk_manager: Optional[RiskConfig] = None

    def to_proto(self):
        """Convert to a RegisterStrategyRequest protobuf message."""
        from gen.python.contracts import execution_service_pb2

        _gateway_map = {
            StrategyConfig.Gateway.GRPC_ADAPTER: execution_service_pb2.GrpcAdapterGW,
            StrategyConfig.Gateway.PAPER_TRADING: execution_service_pb2.PaperTradingGW,
        }
        _feed_map = {
            MarketData.Feed.SIMULATED: execution_service_pb2.SimulatedFeed,
            MarketData.Feed.ALPACA: execution_service_pb2.AlpacaFeed,
            MarketData.Feed.ADAPTER: execution_service_pb2.AdapterFeed,
        }

        req = execution_service_pb2.RegisterStrategyRequest()
        req.strategy_id = self.strategy_id
        req.account_id = self.account_id
        req.gateway = _gateway_map[self.gateway]

        # Adapter config
        if self.adapter is not None:
            req.adapter.venue = self.adapter.venue
            req.adapter.credentials_path = self.adapter.credentials_path
            req.adapter.port = self.adapter.port

        # Market data config
        if self.market_data is not None:
            req.market_data.feed = _feed_map[self.market_data.feed]
            for sub in self.market_data.subscriptions:
                s = req.market_data.subscriptions.add()
                s.symbol = sub.symbol
                if sub.period:
                    s.period = sub.period

        # Risk config
        if self.risk_manager is not None:
            req.risk_params.max_quantity = self.risk_manager.max_quantity
            req.risk_params.max_open_orders = self.risk_manager.max_open_orders
            req.risk_params.daily_pnl_limit = self.risk_manager.daily_pnl_limit

        return req
