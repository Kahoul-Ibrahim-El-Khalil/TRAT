cmake_minimum_required(VERSION 3.25)
project(DrogonRatServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(DEBUG)
  add_compile_options(-g -O0 -Wall -Wextra -Werror)
  add_compile_definitions(DEBUG=1)
endif()

find_package(Drogon REQUIRED)
find_package(fmt REQUIRED)
find_package(OpenSSL REQUIRED)

if(DEFINED ENV{CONDA_PREFIX})
    set(TINYPROC_ROOT "$ENV{CONDA_PREFIX}")
else()
    message(WARNING "CONDA_PREFIX not set. Falling back to system paths for TinyProcessLibrary.")
endif()

set(TINYPROC_INCLUDE_DIR "${TINYPROC_ROOT}/include")
set(TINYPROC_LIB "${TINYPROC_ROOT}/lib/libtiny-process-library.a")

add_library(TinyProcessLibrary STATIC IMPORTED)

set_target_properties(TinyProcessLibrary PROPERTIES
    IMPORTED_LOCATION "${TINYPROC_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${TINYPROC_INCLUDE_DIR}"
)

message(STATUS "Using TinyProcessLibrary:")
message(STATUS "  Include dir: ${TINYPROC_INCLUDE_DIR}")
message(STATUS "  Library: ${TINYPROC_LIB}")

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
)

set(SRCS
    src/Server/constructor.cpp
    src/Server/methods.cpp
    src/Handler/constructor.cpp
    src/Handler/TelegramBotApiMethods/handleFileQueryById.cpp
    src/Handler/TelegramBotApiMethods/handleSendDocument.cpp
    src/Handler/TelegramBotApiMethods/handleSendMessage.cpp
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
        TinyProcessLibrary    # ✅ match the name above exactly
)

include(cmake/Windows.cmake)
