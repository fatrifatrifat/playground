cmake_minimum_required(VERSION 3.23)
project(my_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23 CACHE STRING "C++ version selection")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(my_app src/main.cpp)
install(TARGETS my_app DESTINATION .)

# include(FetchContent)
# 
# set(ALPACA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
# set(ALPACA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
# set(ALPACA_ENABLE_SSL ON CACHE BOOL "" FORCE)
# 
# FetchContent_Declare(
#   alpaca_sdk
#   GIT_REPOSITORY https://github.com/fatrifatrifat/alpaca-sdk-cpp.git
#   GIT_TAG master
# )
# FetchContent_MakeAvailable(alpaca_sdk)
# 
# add_executable(my_app src/main.cpp)
# target_link_libraries(my_app PRIVATE alpaca::alpaca_sdk)
