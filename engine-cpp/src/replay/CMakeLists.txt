cmake_minimum_required(VERSION 3.24)

add_library(trading_replay STATIC
    historical_replay_engine.cpp
)

add_library(trading::replay ALIAS trading_replay)

target_link_libraries(trading_replay
    PUBLIC
        trading_interfaces
        Threads::Threads
)

trading_apply_warnings(trading_replay)
