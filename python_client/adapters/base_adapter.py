
import abc
from dataclasses import dataclass
from enum import StrEnum
from typing import List

class Side(StrEnum):
    UNKNOWN_SIDE = "unknown_side"
    BUY = "buy"
    SELL = "sell"

class OrderType(StrEnum):
    UNKNOWN_TYPE = "unknown_type"
    MARKET = "market"
    LIMIT = "limit"
    STOP = "stop"
    STOP_LIMIT = "stop_limit"

class OrderStatus(StrEnum):
    NEW = "new"
    SUBMITTED = "submitted"
    PARTIAL_FILL = "partial_fill"
    FILLED = "filled"
    CANCELLED = "cancelled"
    REJECTED = "rejected"

class TimeInForce(StrEnum):
    DAY = "day"
    GTC = "gtc" # good till cancelled
    IOC = "ioc" # immediate or cancel
    FOK = "fok" # fill or kill

@dataclass
class SymbolSubscription():
    symbol: str
    bar_periods: List[str]

@dataclass
class Order():
    id: str
    symbol: str
    side: Side
    quantity: float
    price: float
    type: OrderType
    status: OrderStatus
    time_in_force: TimeInForce
    account_id: str
    strategy_id: str
    created_at: str

class BaseAdapter(abc.ABC):
    def __init__(
        self,
        ) -> None:
        pass

    def on_client_submission(self, strategy_id: str, order: Order):
        """Process client submission (ex: submit their order to the broker of this current adapter)"""
    
    def on_client_cancel(self, strategy_id: str, broker_order_id: str):
        """Process client cancellation request with this adapter's broker"""
    
    def on_client_replace_order(self, strategy_id: str, broker_order_id: str, new_order: Order):
        """Replace an a client's order with a new order on this adapter's broker"""
    
    # ???
    def process_stream_fill_request(self, strategy_id: str):
        """Start fill update streaming for a specific strategy on request"""
    
    # ???
    def process_market_data_stream_request(self, subscriptions: List[SymbolSubscription]):
        """Fetch market data and start streaming to client"""

    def on_ping(self):
        """Respond to ping from client"""