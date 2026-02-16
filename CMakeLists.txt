cmake_minimum_required(VERSION 3.26)
project(club_trading LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(ProjectOptions)

club_global_options()

# Prefer upstream config package first
find_package(Protobuf CONFIG QUIET)
if(NOT Protobuf_FOUND)
  find_package(Protobuf REQUIRED)  # fallback
endif()

find_package(absl CONFIG REQUIRED)

add_subdirectory(contracts)
add_subdirectory(engine-cpp)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
