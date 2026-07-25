#!/usr/bin/env python3
"""quarcc trading engine — unified test client.

Usage examples:
  # Run a single strategy (name or 1-based index)
  python3 client.py run 1
  python3 client.py run simple_test_strategy_2 --iterations 50

  # Run all strategies sequentially or in parallel
  python3 client.py run-all
  python3 client.py run-all --parallel

  # Query positions
  python3 client.py positions

  # Kill switch
  python3 client.py kill --reason "done testing" --by dev

  # Print config with gateway overridden (pipe to engine)
  python3 client.py gen-config --gateway alpaca > /tmp/quarcc.yaml
  ./build/engine-cpp/src/trading_engine /tmp/quarcc.yaml
"""

import logging

# Set WARNING *before* importing grpc_interface so its logging.basicConfig() is a no-op.
logging.basicConfig(level=logging.WARNING)

import argparse
import os
import sys
import threading
import time

import yaml

_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, _HERE)               # for grpc_interface
sys.path.insert(0, os.path.dirname(_HERE))  # for gen.python.contracts

import grpc_interface

# Defaults (used when config.yaml has no `test` section for a strategy)
DEFAULT_SERVER = "localhost:50051"
DEFAULT_ORDERS = [
    {"side": "BUY",  "qty": 2.0},
    {"side": "SELL", "qty": 1.5},
]

# Lock so parallel strategy output lines don't interleave mid-line.
_PRINT_LOCK = threading.Lock()

def _print(*args_, **kwargs):
    with _PRINT_LOCK:
        print(*args_, **kwargs)

# Core run logic
def run_strategy(
    client: grpc_interface.ExecutionClient,
    strategy: str,
    *,
    verbose: bool = False,
) -> None:
    if verbose:
        logging.getLogger("grpc_interface").setLevel(logging.INFO)

    strat_id  = strategy + '_id'
    symbol    = "ACDC"
    orders    = DEFAULT_ORDERS

    _print(f"[{strat_id}] {symbol} for {len(orders)} orders")

    t0 = time.perf_counter()
    for _ in range(20):
        for o in orders:
            client.submit_signal(strat_id, symbol, o["side"], float(o["qty"]))
    elapsed = time.perf_counter() - t0

    _print(f"[{strat_id}] submitted {20 * len(orders)} signals in {elapsed:.3f}s")

    pos = client.get_position(symbol)
    if pos:
        sign = "+" if pos["realized_pnl"] >= 0 else ""
        _print(
            f"[{strat_id}] {symbol}  "
            f"qty={pos['quantity']:+.4f}  "
            f"avg={pos['avg_price']:.4f}  "
            f"rPnL={sign}{pos['realized_pnl']:.4f}"
        )


def cmd_run_all(args) -> None:
    if not args.strategies:
        _die(f"no strategies defined")

    strategies = args.strategies
    
    _check_server(args.server)

    if args.parallel:
        errors: list[str] = []
        err_lock = threading.Lock()

        def _run_one(strat: str) -> None:
            c = grpc_interface.ExecutionClient(args.server)
            try:
                run_strategy(
                    c, strat,
                    verbose=args.verbose,
                )
            except Exception as exc:  # noqa: BLE001
                with err_lock:
                    errors.append(f"[{strat}] {exc}")
            finally:
                c.close()

        threads = [
            threading.Thread(target=_run_one, args=(s,), daemon=True)
            for s in strategies
        ]
        for t in threads:
            t.start()
        for t in threads:
            t.join()

        if errors:
            for e in errors:
                print(e, file=sys.stderr)
            sys.exit(1)
    else:
        client = grpc_interface.ExecutionClient(args.server)
        try:
            for strat in strategies:
                run_strategy(
                    client, strat,
                    verbose=args.verbose,
                )
        finally:
            client.close()

# Helpers
def _check_server(address: str) -> None:
    """Exit with a clear message if the engine is not reachable."""
    import grpc as _grpc
    ch = _grpc.insecure_channel(address)
    try:
        _grpc.channel_ready_future(ch).result(timeout=2.0)
    except Exception:
        ch.close()
        _die(f"cannot connect to engine at {address} — is it running?")
    finally:
        ch.close()


def _die(msg: str) -> None:
    print(f"error: {msg}", file=sys.stderr)
    sys.exit(1)

def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="client.py",
        description="quarcc trading engine — test client",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    p.add_argument("--server", default=DEFAULT_SERVER, metavar="HOST:PORT",
                   help=f"gRPC address (default: {DEFAULT_SERVER})")

    sub = p.add_subparsers(dest="cmd", required=True)

    # run
    pr = sub.add_parser("run", help="run test scenario for one or more strategies")
    pr.add_argument("strategies", nargs='+',
                    help="*.py strategy file(s)")
    pr.add_argument("--parallel", action="store_true",
                    help="run all strategies concurrently")
    pr.add_argument("--verbose",    action="store_true",
                   help="show per-signal gRPC logs")
    pr.set_defaults(func=cmd_run_all)

    return p


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
