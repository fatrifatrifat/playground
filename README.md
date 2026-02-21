## Install

**Installing dependencies**

Arch:
```bash
sudo pacman -S websocketpp protobuf grpc
```

Build Server:
```bash
cmake -S . -B build
cmake --build build -j
```

Build Client:
```bash
python -m pip install --upgrade pip
python -m pip install grpcio grpcio-tools protobuf
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
cd playground/python_client/
python3 client.py
```
