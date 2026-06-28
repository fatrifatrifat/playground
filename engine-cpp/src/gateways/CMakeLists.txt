cmake_minimum_required(VERSION 3.24)

add_library(trading_gateways STATIC
    paper_trading_gateway.cpp
    simulated_feed.cpp
    grpc_gateway.cpp
    grpc_market_data_feed.cpp
)

if(TRADING_ENABLE_ALPACA_SDK)
    target_sources(trading_gateways PRIVATE alpaca_fix_gateway.cpp)
endif()

add_library(trading::gateways ALIAS trading_gateways)

target_link_libraries(trading_gateways
    PUBLIC
        trading_interfaces
        Threads::Threads
)

trading_apply_warnings(trading_gateways)
