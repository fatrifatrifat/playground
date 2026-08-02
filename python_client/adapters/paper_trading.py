import logging
import uuid

from .base_adapter import BaseAdapter
from gen.python.contracts import order_pb2

logger = logging.getLogger(__name__)


class PaperAdapter(BaseAdapter):
    def __init__(self, port: int = 50052):
        super().__init__(port)

    def _connect(self) -> None:
        logger.info("PaperAdapter: connected")

    def _disconnect(self) -> None:
        logger.info("PaperAdapter: disconnected")

    def _is_connected(self) -> bool:
        return True

    def _submit_order(self, order: order_pb2.Order) -> tuple[str, bool, str]:
        broker_id = str(uuid.uuid4())
        logger.info(
            "PaperAdapter: submit order symbol=%s side=%s qty=%s -> broker_id=%s",
            order.symbol, order.side, order.quantity, broker_id,
        )
        return broker_id, True, ""

    def _replace_order(self, broker_order_id: str, new_order: order_pb2.Order) -> tuple[str, bool, str]:
        new_broker_id = str(uuid.uuid4())
        logger.info(
            "PaperAdapter: replace %s -> %s symbol=%s qty=%s",
            broker_order_id, new_broker_id, new_order.symbol, new_order.quantity,
        )
        return new_broker_id, True, ""

    def _cancel_order(self, broker_order_id: str) -> tuple[bool, str]:
        logger.info("PaperAdapter: cancel %s", broker_order_id)
        return True, ""

    def _start_market_data(self, subscriptions: list) -> None:
        logger.info("PaperAdapter: _start_market_data called (%d subscriptions)", len(subscriptions))

    def _stop_market_data(self) -> None:
        logger.info("PaperAdapter: _stop_market_data called")
