cmake_minimum_required(VERSION 3.24)

add_library(trading_core STATIC
    order_manager.cpp
    position_keeper.cpp
    risk_manager.cpp
    poll_scheduler.cpp
    feed_registry.cpp
    adapter_manager.cpp
)

add_library(trading::core ALIAS trading_core)

target_link_libraries(trading_core
    PUBLIC
        trading_interfaces
    PRIVATE
        Threads::Threads
)

trading_apply_warnings(trading_core)

add_library(trading_engine STATIC
    trading_engine.cpp
)

add_library(trading::engine ALIAS trading_engine)

target_link_libraries(trading_engine
    PUBLIC
        trading_core
    PRIVATE
        trading_gateways
        trading_grpc
        trading_persistence
        Threads::Threads
)

trading_apply_warnings(trading_engine)
