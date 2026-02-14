cmake_minimum_required(VERSION 3.23)
project(arima_forecast LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(pybind11 CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)

pybind11_add_module(arima_forecast
  bindings.cpp
  arima.cpp
)

target_include_directories(arima_forecast PRIVATE ${EIGEN3_INCLUDE_DIRS})
target_link_libraries(arima_forecast PRIVATE Eigen3::Eigen)
