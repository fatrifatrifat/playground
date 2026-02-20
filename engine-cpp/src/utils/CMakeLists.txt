cmake_minimum_required(VERSION 3.24)

add_library(trading_utils STATIC
    order_id_generator.cpp
)

add_library(trading::utils ALIAS trading_utils)

target_link_libraries(trading_utils
    PUBLIC
        trading_interfaces
        Threads::Threads
)

trading_apply_warnings(trading_utils)
