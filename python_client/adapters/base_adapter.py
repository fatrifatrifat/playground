
import abc
import logging
import queue
import threading
import time
from concurrent import futures
from typing import List, Optional

import grpc

import sys, os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "../..")))

from gen.python.contracts import (
    gateway_adapter_service_pb2_grpc as svc_grpc,
    common_pb2,
    execution_pb2,
    market_data_pb2,
    order_pb2,
)
from gen.python.contracts.gateway_adapter_service_pb2 import (
    AdapterSubmitResponse,
    AdapterCancelResponse,
    AdapterReplaceResponse,
    PingResponse,
)

logger = logging.getLogger(__name__)

class BaseAdapter(abc.ABC):
    """
    Methods to override:
    _connect -> connects to the broker
    _disconnect -> disconnects from the broker
    _is_connected -> return True if currently connected
    _submit_order -> sends an order signal to the broker
    _replace_order -> replaces an order
    _cancel_order -> cancels an order
    _start_market_data -> starts the market data stream
    _stop_market_data -> stops the market data stream

    # No `_start/stop_fill_stream` needed, because that's going on
    # as long as we're connected to the broker
    """
    def __init__(self, port: int = 50052) -> None:
        self._port = port
        self._server: Optional[grpc.Server] = None
        self._connected = False
        self._stop_event = threading.Event()

        self._order_routing: dict[str, str] = {}
        self._order_routing_lock = threading.Lock()

        self._fill_queues: dict[str, queue.Queue] = {}
        self._fill_queues_lock = threading.Lock()

        self._seen_execution_ids: set[str] = set()
        self._seen_execution_ids_lock = threading.Lock()

        self._md_queues: list[queue.Queue] = []
        self._md_queues_lock = threading.Lock()

        self._reconnect_thread = threading.Thread(
                target=self.reconnect_loop, daemon=True, name="adapter-reconnect"
        )

    ### Abstract methods to override ###
    @abc.abstractmethod
    def _connect(self) -> None:
        """Connects to the broker"""

    @abc.abstractmethod
    def _disconnect(self) -> None:
        """Disconnects to the broker"""

    @abc.abstractmethod
    def _is_connected(self) -> bool:
        """Returns True if the broker is connected"""

    @abc.abstractmethod
    def _submit_order(self, order: order_pb2.Order) -> tuple[str, bool, str]:
        """Submits an order to the broker"""

    @abc.abstractmethod
    def _replace_order(self, broker_order_id: str, new_order: order_pb2.Order) -> tuple[str, bool, str]:
        """Replaces an order to the broker"""

    @abc.abstractmethod
    def _cancel_order(self, broker_order_id: str) -> tuple[bool, str]:
        """Cancels an order to the broker"""

    @abc.abstractmethod
    def _start_market_data(self, subscriptions: list) -> None:
        """Starts getting market data from the broker"""

    @abc.abstractmethod
    def _stop_market_data(self) -> None:
        """Stops getting market data from the broker"""

    ### gRPC Implementation ###
    def SubmitOrder(self, request, context):
        strategy_id = request.strategy_id
        broker_id, accepted, reason = self._submit_order(request.order)
        if accepted:
           with self._order_routing_lock:
               self._order_routing[broker_id] = strategy_id
        return AdapterSubmitResponse(
            accepted=accepted,
            broker_order_id=broker_id,
            rejection_reason=reason
        )

    def CancelOrder(self, request, context):
        accepted, reason = self._cancel_order(request.broker_order_id)
        return AdapterCancelResponse(
            accepted=accepted,
            rejection_reason=reason
        )

    def ReplaceOrder(self, request, context):
        new_broker_order_id, accepted, reason = self._replace_order(
            request.broker_order_id, request.new_order
        )
        if accepted:
            with self._order_routing_lock:
                self._order_routing.pop(request.broker_order_id, None)
                self._order_routing[new_broker_order_id] = request.strategy_id
        return AdapterReplaceResponse(
            accepted=accepted,
            new_broker_order_id=new_broker_order_id,
            rejection_reason=reason,
        )

    def StreamFills(self, request, context):
        strategy_id = request.strategy_id
        q: queue.Queue = queue.Queue()

        with self._fill_queues_lock:
            self._fill_queues[strategy_id] = q

        logger.info("StreamFills opened for strategy_id=%s", strategy_id)
        try:
            while context.is_active() and not self._stop_event.is_set():
                try:
                    report = q.get(timeout=1.0)
                except queue.Empty:
                    continue
                if report is None:
                    break
                yield report
        finally:
            with self._fill_queues_lock:
                self._fill_queues.pop(strategy_id, None)
            logger.info("StreamFills closed for strategy_id=%s", strategy_id)

    def StreamMarketData(self, request, context):
        q: queue.Queue = queue.Queue()

        with self._md_queues_lock:
            self._md_queues.append(q)

        subscriptions = list(request.subscriptions)
        self._start_market_data(subscriptions)

        logger.info("StreamMarketData opened (%d symbols)", len(subscriptions))
        try:
            while context.is_active() and not self._stop_event.is_set():
                try:
                    event = q.get(timeout=1.0)
                except queue.Empty:
                    continue
                if event is None:
                    break
                yield event
        finally:
            with self._md_queues_lock:
                try:
                    self._md_queues.remove(q)
                except ValueError:
                    pass
            logger.info("StreamMarketData closed")

    def Ping(self, request, context):
        ready = self._is_connected()
        msg = "ready" if ready else "broker not connected"
        return PingResponse(ready=ready, status_message=msg)

    ### Reconnect Loop ###
    def reconnect_loop(self) -> None:
        """Reconnects to the broker"""
        backoff = 1.0
        while not self._stop_event.is_set():
            if not self._is_connected():
                logger.info("Broker disconnected; Reconnecting (backoff=%.1fs)", backoff)
                self._connected = False
                try:
                    self._connect()
                    self._connected = True
                    backoff = 1.0
                    logger.info("Broker reconnected")
                except Exception as exc:
                    logger.warning("Reconnect failed: %s", exc)
                    self._stop_event.wait(backoff)
                    backoff = min(backoff * 2, 60.0)
                    continue

            self._stop_event.wait(5.0)

    ### Core ###
    def run(self) -> None:
        try:
            self._connect()
            self._connected = True
            logger.info("Broker connection established")
        except Exception as exc:
            logger.warning("Broker connection failed: %s; Retrying...", exc)
            self._connected = False

        self._reconnect_thread.start()
        self._server = grpc.server(
            futures.ThreadPoolExecutor(max_workers=20),
            options=[
                ("grpc.max_send_message_length", 64 * 1024 * 1024),
                ("grpc.max_receive_message_length", 64 * 1024 * 1024),
            ],
        )
        svc_grpc.add_GatewayAdapterServiceServicer_to_server(self, self._server)
        self._server.add_insecure_port(f"0.0.0.0:{self._port}")
        self._server.start()
        logger.info("Adapter gRPC server listening on port %d", self._port)

        self._server.wait_for_termination()

    def stop(self) -> None:
        """Ends all streaming loops, and stops the gRPC server"""
        self._stop_event.set()

        with self._fill_queues_lock:
            for q in self._fill_queues.values():
                q.put(None)

        with self._md_queues_lock:
            for q in self._md_queues:
                q.put(None)

        try:
            self._stop_market_data()
        except Exception:
            pass

        try:
            self._disconnect()
        except Exception:
            pass

        if self._server:
            self._server.stop(grace=2)
            self._server = None
