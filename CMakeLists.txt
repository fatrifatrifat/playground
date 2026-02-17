cmake_minimum_required(VERSION 3.23)
project(my_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find installed packages
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# Paths
set(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto")
set(PROTO_FILE "${PROTO_DIR}/helloworld.proto")
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

# Main target
add_executable(my_app main.cpp)

# Generate *.pb.cc / *.pb.h
protobuf_generate(
  TARGET my_app
  LANGUAGE cpp
  PROTOS "${PROTO_FILE}"
  IMPORT_DIRS "${PROTO_DIR}"
  PROTOC_OUT_DIR "${GEN_DIR}"
)

# Generate *.grpc.pb.cc / *.grpc.pb.h
protobuf_generate(
  TARGET my_app
  LANGUAGE grpc
  PROTOS "${PROTO_FILE}"
  IMPORT_DIRS "${PROTO_DIR}"
  PROTOC_OUT_DIR "${GEN_DIR}"
  GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
  PLUGIN "protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
)

# Include generated headers
target_include_directories(my_app PRIVATE "${GEN_DIR}")

# Link runtime libs
target_link_libraries(my_app
  PRIVATE
    protobuf::libprotobuf
    gRPC::grpc++
)

install(TARGETS my_app RUNTIME DESTINATION bin)
