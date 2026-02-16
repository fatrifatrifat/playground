cmake_minimum_required(VERSION 3.24)

project(trading_system
    VERSION 0.1.0
    DESCRIPTION "Modular trading system (C++ + Protobuf contracts)"
    LANGUAGES CXX
)

# ---- Global language/build settings ----
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

# ---- Feature toggles ----
option(TRADING_BUILD_APP               "Build engine-cpp/src/main.cpp executable" ON)
option(TRADING_ENABLE_GRPC             "Generate gRPC code from .proto files" OFF)
option(TRADING_ENABLE_FIX_GATEWAY      "Enable Alpaca FIX gateway (QuickFIX required)" ON)
option(TRADING_ENABLE_WS_GATEWAY       "Enable websocket market data gateway (websocketpp required)" ON)
option(TRADING_ENABLE_PROMETHEUS       "Enable Prometheus exporter (prometheus-cpp required)" ON)

add_subdirectory(contracts)
add_subdirectory(engine-cpp)
