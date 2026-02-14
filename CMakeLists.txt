cmake_minimum_required(VERSION 3.23)
project(quant_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23 CACHE STRING "C++ version selection")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(quant_engine src/main.cpp)
install(TARGETS quant_engine DESTINATION .)
