set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# === Precompiled headers (only once) ===
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
#third party libraries
	<curl/curl.h>
	<fmt/core.h>
	<fmt/chrono.h>
)

# === Output directories ===
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# === Options ===
option(STATIC "Build static libraries" ON)
option(DEBUG "Enable debug logging" OFF)

option(DEBUG_RAT_NETWORKING "Enable debug logs in networking module" OFF)
option(DEBUG_RAT_SYSTEM "Enable debug logs in system module" OFF)
option(DEBUG_RAT_PROCESS "Enable debug logs in process module" OFF)
option(DEBUG_RAT_TBOT "Enable debug logs in Telegram bot module" OFF)
option(DEBUG_RAT_HANDLER "Enable debug logs in handler module" OFF)
option(DEBUG_RAT_THREADING "Enable debug logs in threading module" OFF)

# === Dependencies ===
find_package(fmt REQUIRED)
find_package(CURL REQUIRED)

if(DEBUG OR DEBUG_RAT_NETWORKING OR DEBUG_RAT_SYSTEM OR DEBUG_RAT_PROCESS OR DEBUG_RAT_TBOT OR DEBUG_RAT_HANDLER OR DEBUG_RAT_THREADING)
    find_package(spdlog REQUIRED)
    message(STATUS "Debug mode enabled: adding -g -O0")
    add_compile_definitions(
        $<$<BOOL:${DEBUG_RAT_NETWORKING}>:DEBUG_RAT_NETWORKING>
        $<$<BOOL:${DEBUG_RAT_SYSTEM}>:DEBUG_RAT_SYSTEM>
        $<$<BOOL:${DEBUG_RAT_PROCESS}>:DEBUG_RAT_PROCESS>
        $<$<BOOL:${DEBUG_RAT_TBOT}>:DEBUG_RAT_TBOT>
        $<$<BOOL:${DEBUG_RAT_HANDLER}>:DEBUG_RAT_HANDLER>
        $<$<BOOL:${DEBUG_RAT_THREADING}>:DEBUG_RAT_THREADING>
    )
    add_compile_options(-Wall -Werror -Wpedantic -g -O0)
else()
    add_compile_options(-O3)
endif()

# === Submodules ===
add_subdirectory(simdjson)
add_subdirectory(tiny-process-library)
add_subdirectory(rat_encryption)
add_subdirectory(rat_networking)
add_subdirectory(rat_threading)
add_subdirectory(rat_system)
add_subdirectory(rat_tbot)
add_subdirectory(rat_media)
add_subdirectory(rat_handler)
add_subdirectory(rat_trat)

# === Attach PCH to all in-tree targets (but not system libs like simdjson) ===
foreach(tgt IN ITEMS
    rat_encryption
    rat_networking
    rat_threading
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

