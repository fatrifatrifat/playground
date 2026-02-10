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
