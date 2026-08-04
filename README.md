## quarcc-trading-engine

An event driven trading engine written in C++23. Strategy logic runs in Python and communicates with the execution core over gRPC. The engine handles order routing, position tracking, risk checks, market data fan out, and persistence.
It keeps all the necessary high performance processing in C++, while keeping a simple strategy building interface.

---

## Architecture

```
Python strategy process
        │
        │  gRPC (ExecutionService)
        ▼
  ┌───────────────────────────────────────────┐
  │              TradingEngine                │
  │  ┌──────────┐   ┌──────────────────────┐  │
  │  │gRPCServer│   │     FeedRegistry     │  │
  │  └────┬─────┘   │  (routes bars/ticks  │  │
  │       │         │   to OrderManagers)  │  │
  │       │         └──────────┬───────────┘  │
  │       ▼                    │              │
  │  ┌────────────────────┐    │              │
  │  │   OrderManager     │◄───┘              │
  │  │  (per strategy)    │                   │
  │  │  ┌──────────────┐  │                   │
  │  │  │PositionKeeper│  │                   │
  │  │  ├──────────────┤  │                   │
  │  │  │  RiskManager │  │                   │
  │  │  ├──────────────┤  │                   │
  │  │  │   IJournal   │  │                   │
  │  │  ├──────────────┤  │                   │
  │  │  │ IOrderStore  │  │                   │
  │  │  ├──────────────┤  │                   │
  │  │  │ IExecutionGw │  │                   │
  │  │  └──────────────┘  │                   │
  │  └────────────────────┘                   │
  └───────────────────────────────────────────┘
```

## Prerequisites

- CMake ≥ 3.24
- C++23 compiler: GCC 14+ or Clang 17+
- [Conan 2](https://conan.io/) (`pip install conan`)

C++ dependencies (gRPC, protobuf, yaml-cpp, SQLite3, GoogleTest) are fetched and built automatically by Conan so no system packages needed for them.

### Arch

```bash
sudo pacman -S git cmake ninja gcc python-pip
pip install conan
```

---

## Build

### Standard build

```bash
conan install . --output-folder=build --build=missing
cmake --preset conan-release
cmake --build build -j
```

The compiled engine build is in `build/engine-cpp/src/trading_engine`.

### Debug build

```bash
conan install . --output-folder=build --build=missing -s build_type=Debug
cmake --preset conan-debug
cmake --build build -j
```

---

## Running

Start the engine (from the repo root):

```bash
./build/engine-cpp/src/trading_engine
```

In a separate terminal, run the Python client:

```bash
bash ./scripts/generate_protos_python.sh
cd python_client/
pip install -r requirements.txt
python3 momentum.py
```

### Regenerating Python protobuf bindings

If you modify any `.proto` files, regenerate the Python bindings before running the client:

```bash
bash ./scripts/generate_protos_python.sh
```
