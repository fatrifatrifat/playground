cmake_minimum_required(VERSION 3.24)

project(trading_system
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Important so system installed and conan installed grpc's don't "collide"
foreach(_conan_lib_dir ${CMAKE_LIBRARY_PATH})
    get_filename_component(_conan_pkg_root "${_conan_lib_dir}/.." ABSOLUTE)
    if(IS_DIRECTORY "${_conan_pkg_root}/bin")
        list(PREPEND CMAKE_PROGRAM_PATH "${_conan_pkg_root}/bin")
    endif()
endforeach()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

option(TRADING_ENABLE_ALPACA_SDK "Enable Alpaca SDK integration" ON)
option(TRADING_BUILD_TESTS       "Build unit tests (Google Test + Google Mock)" ON)

if(TRADING_ENABLE_ALPACA_SDK)
    include(FetchContent)
    set(ALPACA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(ALPACA_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(ALPACA_ENABLE_SSL     ON  CACHE BOOL "" FORCE)
    FetchContent_Declare(
        alpaca_sdk
        GIT_REPOSITORY https://github.com/fatrifatrifat/alpaca-sdk-cpp.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(alpaca_sdk)
endif()

add_subdirectory(contracts)
add_subdirectory(engine-cpp)
