cmake_minimum_required(VERSION 3.24)

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC     CONFIG REQUIRED)

set(TRADING_PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/common.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/strategy_signal.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/order.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/execution.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/market_data.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/execution_service.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/gateway_adapter_service.proto
)

set(TRADING_PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${TRADING_PROTO_GEN_DIR})

add_library(trading_contracts STATIC)
add_library(trading::contracts ALIAS trading_contracts)

target_link_libraries(trading_contracts PUBLIC protobuf::libprotobuf gRPC::grpc++ gRPC::grpc)

protobuf_generate(
    TARGET         trading_contracts
    LANGUAGE       cpp
    PROTOS         ${TRADING_PROTO_FILES}
    IMPORT_DIRS    ${CMAKE_CURRENT_SOURCE_DIR}
    PROTOC_OUT_DIR ${TRADING_PROTO_GEN_DIR}
)

if(TARGET gRPC::grpc_cpp_plugin)
    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
else()
    find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin REQUIRED)
endif()

protobuf_generate(
    TARGET               trading_contracts
    LANGUAGE             grpc
    GENERATE_EXTENSIONS  .grpc.pb.h .grpc.pb.cc
    PLUGIN               "protoc-gen-grpc=${_GRPC_CPP_PLUGIN_EXECUTABLE}"
    PROTOS               ${TRADING_PROTO_FILES}
    IMPORT_DIRS          ${CMAKE_CURRENT_SOURCE_DIR}
    PROTOC_OUT_DIR       ${TRADING_PROTO_GEN_DIR}
)

target_include_directories(trading_contracts
    PUBLIC
        $<BUILD_INTERFACE:${TRADING_PROTO_GEN_DIR}>
        $<INSTALL_INTERFACE:include/trading/contracts>
)
