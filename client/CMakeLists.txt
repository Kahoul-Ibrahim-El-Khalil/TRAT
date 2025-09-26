cmake_minimum_required(VERSION 3.25)
project(rat_project CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

include(cmake/LinuxLibs.cmake)
include(cmake/WindowsLibs.cmake)
include(cmake/PrecompiledHeadersConfig.cmake)
option(BUILD_SHARED_LIBS "Building static" OFF)
option(DEBUG "Enable debug mode" OFF)

option(DEBUG_RAT_SYSTEM "Enable RAT system debug logging" OFF)
option(DEBUG_RAT_HANDLER "Enable RAT handler debug logging" OFF)
option(DEBUG_RAT_NETWORKING "Enable RAT networking debug logging" OFF)
option(DEBUG_RAT_TBOT "Enable RAT TBOT debug logging" OFF)
option(DEBUG_RAT_THREADING "Enable RAT threading debug logging" OFF)
option(BUILD_DUMMY_BOT "Build a test bot")

foreach(opt IN ITEMS DEBUG_RAT_SYSTEM DEBUG_RAT_HANDLER DEBUG_RAT_NETWORKING DEBUG_RAT_TBOT DEBUG_RAT_THREADING)
    if(${opt})
        add_compile_definitions(${opt})
    endif()
endforeach()

add_subdirectory(rat_encryption)
add_subdirectory(rat_compression)
add_subdirectory(rat_threading)
add_subdirectory(rat_networking)
add_subdirectory(rat_system)
add_subdirectory(rat_tbot)
add_subdirectory(rat_media)
add_subdirectory(rat_handler)
add_subdirectory(rat_trat)

foreach(tgt IN ITEMS
    rat_encryption rat_compression rat_threading
    rat_networking rat_system rat_tbot
    rat_media rat_handler
)
    if(TARGET ${tgt})
        target_link_libraries(${tgt} PUBLIC rat_precompiled_headers)
    endif()
endforeach()
