cmake_minimum_required(VERSION 3.25)
project(rat_suite LANGUAGES CXX)

# Use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# === Options ===
option(STATIC "Build static libraries" ON)
option(DEBUG "Enable debug logging" OFF)

# === Toolchain (MinGW) ===
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Optional: static linking of GCC/StdC++
if(STATIC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++")
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# === Dependency paths ===
set(WIN_LIB_DIR "$ENV{HOME}/.windows")

# --- fmt ---
set(FMT_INCLUDE_DIR "${WIN_LIB_DIR}/fmt/include")
set(FMT_LIB_DIR "${WIN_LIB_DIR}/fmt/lib")
find_path(FMT_INCLUDE_DIR fmt/core.h PATHS ${FMT_INCLUDE_DIR})
find_library(FMT_LIBRARY NAMES fmt PATHS ${FMT_LIB_DIR})
add_library(fmt::fmt STATIC IMPORTED)
set_target_properties(fmt::fmt PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${FMT_INCLUDE_DIR}
    IMPORTED_LOCATION ${FMT_LIBRARY}
)

# --- curl ---
set(CURL_INCLUDE_DIR "${WIN_LIB_DIR}/curl/include")
set(CURL_LIB_DIR "${WIN_LIB_DIR}/curl/lib")
find_path(CURL_INCLUDE_DIR curl/curl.h PATHS ${CURL_INCLUDE_DIR})
find_library(CURL_LIBRARY NAMES curl PATHS ${CURL_LIB_DIR})
add_library(CURL::libcurl STATIC IMPORTED)
set_target_properties(CURL::libcurl PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${CURL_INCLUDE_DIR}
    IMPORTED_LOCATION ${CURL_LIBRARY}
)

# --- zlib (same as curl path) ---
find_path(ZLIB_INCLUDE_DIR zlib.h PATHS ${CURL_INCLUDE_DIR})
find_library(ZLIB_LIBRARY NAMES z zlib PATHS ${CURL_LIB_DIR})
add_library(ZLIB::ZLIB STATIC IMPORTED)
set_target_properties(ZLIB::ZLIB PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${ZLIB_INCLUDE_DIR}
    IMPORTED_LOCATION ${ZLIB_LIBRARY}
)

# spdlog (optional)
if(DEBUG)
    find_package(spdlog REQUIRED)
endif()

# === Submodules ===
add_subdirectory(rat_compression)
add_subdirectory(rat_encryption)
add_subdirectory(rat_networking)
add_subdirectory(rat_system)
add_subdirectory(rat_tbot)
add_subdirectory(rat_handler)
add_subdirectory(rat_trat)

# === Link system libraries ===
target_link_libraries(rat_networking PRIVATE ws2_32 wldap32 crypt32 normaliz)
target_link_libraries(rat_system PRIVATE kernel32 user32)

# === Link dependencies to main targets ===
target_link_libraries(rat_networking PRIVATE fmt::fmt CURL::libcurl ZLIB::ZLIB)
