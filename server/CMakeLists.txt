cmake_minimum_required(VERSION 3.25)
project(DrogonRatServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(DEBUG)
  add_compile_options(-g -O0 -Wall -Wextra -Werror)
  add_compile_definitions(DEBUG=1)
endif()

# Find Drogon
find_package(Drogon REQUIRED)
find_package(fmt REQUIRED)
# Find OpenSSL
find_package(OpenSSL REQUIRED)

# Include your headers
include_directories(${CMAKE_SOURCE_DIR}/include
                    ${CMAKE_SOURCE_DIR} #for the models
)

# Sources
set(SRCS
    src/main.cpp
    src/Server/constructor.cpp
    src/Server/methods.cpp
    src/Handler/constructor.cpp
    src/Handler/methods.cpp
    src/Handler/TelegramBotApiMethods.cpp
)

# Create the executable
add_executable(server ${SRCS})

# Precompiled header interface
add_library(PCH INTERFACE)
target_precompile_headers(PCH INTERFACE
    <drogon/HttpAppFramework.h>
    <drogon/HttpResponse.h>
    <drogon/HttpRequest.h>
    <drogon/drogon.h>
    <drogon/orm/DbClient.h>
    <drogon/orm/Mapper.h>
    <functional>
    <fmt/core.h>
    <filesystem>
    <algorithm>
    <string>
    <vector>
    <filesystem>
    <fstream>
    <string_view>
)
target_link_libraries(server
    PRIVATE
        PCH
        Drogon::Drogon
        OpenSSL::SSL
        OpenSSL::Crypto
        fmt::fmt
)

# On Windows/MinGW: copy OpenSSL DLLs into output dir
if(WIN32)
    set(OPENSSL_DLL_DIR "X:/msys2/mingw64/bin")

    add_custom_command(TARGET server POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPENSSL_DLL_DIR}/libcrypto-3-x64.dll"
            $<TARGET_FILE_DIR:server>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPENSSL_DLL_DIR}/libssl-3-x64.dll"
            $<TARGET_FILE_DIR:server>
        COMMENT "Copying OpenSSL DLLs into output directory")
endif()

