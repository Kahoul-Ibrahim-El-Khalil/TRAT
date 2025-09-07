set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX g++)

add_library(rat_precompiled_headers INTERFACE)

target_precompile_headers(rat_precompiled_headers INTERFACE
    <iostream>
    <vector>
    <array>
    <string>
    <filesystem>
    <chrono>
    <future>
    <queue>
    <functional>
    <fstream>
    <sstream>
    <cstdint>
    <cstring>
    <cstddef>
    <cstdio>
    <cstdlib>
    <csignal>
    <map>
    <thread>
    <mutex>
    <memory>
    <algorithm>
# third party
    <curl/curl.h>
    <fmt/core.h>
    <fmt/chrono.h>
	<simdjson.h>
	<tiny-process-library/process.hpp>
# windows-specific
    <windows.h>
)

# === Output directories ===
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# === Options ===
option(STATIC "Build static libraries" ON)
option(DEBUG "Enable debug logging" OFF)

# === Static library path ===
set(STATIC_DIR "${CMAKE_SOURCE_DIR}/../../Static")
set(STATIC_INCLUDE "${STATIC_DIR}/include")
set(STATIC_LIB "${STATIC_DIR}/lib")

include_directories(${STATIC_INCLUDE})
link_directories(${STATIC_LIB})

find_package(Threads REQUIRED)

add_library(fmt STATIC IMPORTED GLOBAL)
set_target_properties(fmt PROPERTIES
    IMPORTED_LOCATION "${STATIC_LIB}/libfmt.a"
    INTERFACE_INCLUDE_DIRECTORIES "${STATIC_INCLUDE}"
)

add_library(zlib STATIC IMPORTED GLOBAL)
set_target_properties(zlib PROPERTIES
    IMPORTED_LOCATION "${STATIC_LIB}/libz.a"
    INTERFACE_INCLUDE_DIRECTORIES "${STATIC_INCLUDE}"
)

add_library(simdjson STATIC IMPORTED GLOBAL)
set_target_properties(simdjson PROPERTIES
    IMPORTED_LOCATION "${STATIC_LIB}/libsimdjson.a"
    INTERFACE_INCLUDE_DIRECTORIES "${STATIC_INCLUDE}"
)

add_library(tiny-process-library STATIC IMPORTED GLOBAL)
set_target_properties(tiny-process-library PROPERTIES
    IMPORTED_LOCATION "${STATIC_LIB}/libtiny-process-library.a"
    INTERFACE_INCLUDE_DIRECTORIES "${STATIC_INCLUDE}"
)

add_library(curl STATIC IMPORTED GLOBAL)
set_target_properties(curl PROPERTIES
    IMPORTED_LOCATION "${STATIC_LIB}/libcurl.a"
    INTERFACE_INCLUDE_DIRECTORIES "${STATIC_INCLUDE}"
    INTERFACE_COMPILE_DEFINITIONS "CURL_STATICLIB"
    INTERFACE_LINK_LIBRARIES "zlib;ws2_32;bcrypt;crypt32;wldap32;normaliz"
)

add_library(CURL::libcurl ALIAS curl)
add_library(FMT::fmt ALIAS fmt)
add_library(ZLIB::zlib ALIAS zlib)
add_library(SIMDJSON::simdjson ALIAS simdjson)
add_library(TINY_PROCESS_LIBRARY::tiny-process-library ALIAS tiny-process-library)

if(DEBUG)
    find_package(spdlog REQUIRED)
    message(STATUS "Debug mode enabled: adding  -g -O0")

    add_compile_options(-Wall -Werror -Wpedantic -g -O0)
else()
    add_compile_options(-O3)
endif()

if(STATIC AND MINGW)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
endif()

# === Submodules ===
add_subdirectory(rat_encryption)
add_subdirectory(rat_compression)
add_subdirectory(rat_threading)
add_subdirectory(rat_networking)
add_subdirectory(rat_system)
add_subdirectory(rat_tbot)
add_subdirectory(rat_media)
add_subdirectory(rat_handler)
add_subdirectory(rat_trat)

# === Attach PCH to all in-tree targets (skip external libs like simdjson) ===
foreach(tgt IN ITEMS
    rat_encryption
    rat_compression
    rat_process
    rat_threading
    rat_networking
    rat_system
    rat_tbot
    rat_media
    rat_handler
    rat_trat
)
    if(TARGET ${tgt})
        target_link_libraries(${tgt} PRIVATE rat_precompiled_headers)
    endif()
endforeach()

# === Global linkage (to reuse in submodules) ===
set(GLOBAL_LIBS fmt curl zlib simdjson tiny-process-library ws2_32 bcrypt crypt32)

