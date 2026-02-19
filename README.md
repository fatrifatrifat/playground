## Install

**Installing dependencies**

Arch:
```bash
sudo pacman -S websocketpp protobuf grpc
```

Build:
```bash
cmake -S . -B build
cmake --build build -j
bash ./scripts/generate_protos_python.py
```

## Usage

Server Terminal:

```bash
cd playground/
./build/engine-cpp/src/trading_engine
```

Client Terminal:

```bash
cd python_client/
python3 client.py
```
