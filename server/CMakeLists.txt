
cmake_minimum_required(VERSION 3.25)
project(rat_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# === Precompiled Headers ===
add_library(rat_precompiled_headers INTERFACE)

target_precompile_headers(rat_precompiled_headers INTERFACE
    <iostream>
    <array>
    <string>
    <functional>
    # third-party libraries
    <drogon/drogon.h>
)

# === Output directories ===
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# === Dependencies ===
find_package(Drogon REQUIRED)
find_package(OpenSSL REQUIRED)

# === Project Paths ===
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)



# === Executable ===
add_executable(server ${SRC_DIR}/main.cpp)
target_include_directories(server PUBLIC ${INCLUDE_DIR})
target_link_libraries(server
    PUBLIC
        Drogon::Drogon
        OpenSSL::SSL
        OpenSSL::Crypto
        rat_precompiled_headers
)

