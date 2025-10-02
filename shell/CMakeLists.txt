cmake_minimum_required(VERSION 3.16)
project(DrogonRatShell LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(DEFINED ENV{CONDA_PREFIX})
    set(_jsoncpp_root "$ENV{CONDA_PREFIX}")
else()
    message(WARNING "CONDA_PREFIX is not set.")
endif()

set(JSONCPP_INCLUDE_DIR "${_jsoncpp_root}/include")
set(JSONCPP_LIBRARY "${_jsoncpp_root}/lib/libjsoncpp.so")

message(STATUS "Using JsonCpp include dir: ${JSONCPP_INCLUDE_DIR}")
message(STATUS "Using JsonCpp library: ${JSONCPP_LIBRARY}")

add_executable(Shell Shell.cpp main.cpp)

target_include_directories(Shell PRIVATE ${JSONCPP_INCLUDE_DIR})
target_link_libraries(Shell PRIVATE ${JSONCPP_LIBRARY})

target_compile_options(Shell PRIVATE -Wall -Wextra -Wpedantic)
if(DEBUG)
    find_package(fmt REQUIRED)
    target_link_libraries(Shell PRIVATE fmt::fmt)
endif()

