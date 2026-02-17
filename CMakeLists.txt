cmake_minimum_required(VERSION 3.23)
project(my_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Prefer package config mode consistently
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

# gRPC examples for system-installed deps use this before Protobuf CONFIG
# (compatibility helper for protobuf CMake integration)
option(protobuf_MODULE_COMPATIBLE TRUE)

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

set(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto")
set(GEN_DIR   "${CMAKE_CURRENT_BINARY_DIR}/generated")

file(GLOB_RECURSE PROTO_FILES CONFIGURE_DEPENDS
  "${PROTO_DIR}/*.proto"
)

if(NOT PROTO_FILES)
  message(FATAL_ERROR "No .proto files found under ${PROTO_DIR}")
endif()

add_executable(my_app
  main.cpp
  ${PROTO_FILES}
)

# Generate *.pb.cc / *.pb.h
protobuf_generate(
  TARGET my_app
  LANGUAGE cpp
  PROTOS ${PROTO_FILES}
  IMPORT_DIRS "${PROTO_DIR}"
  APPEND_PATH
  PROTOC_OUT_DIR "${GEN_DIR}"
)

# Generate *.grpc.pb.cc / *.grpc.pb.h
protobuf_generate(
  TARGET my_app
  LANGUAGE grpc
  PROTOS ${PROTO_FILES}
  IMPORT_DIRS "${PROTO_DIR}"
  APPEND_PATH
  PROTOC_OUT_DIR "${GEN_DIR}"
  GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
  PLUGIN "protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
)

target_include_directories(my_app PRIVATE "${GEN_DIR}")

target_link_libraries(my_app
  PRIVATE
    protobuf::libprotobuf
    gRPC::grpc++
)

install(TARGETS my_app RUNTIME DESTINATION bin)
