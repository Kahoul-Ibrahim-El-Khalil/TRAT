cmake_minimum_required(VERSION 3.25)
project(rat_suite LANGUAGES CXX)

# Choose target
set(TARGET_PLATFORM "LINUX" CACHE STRING "Target platform: LINUX, LINUX_WIN, WIN")

if(TARGET_PLATFORM STREQUAL "LINUX")
    message(STATUS "Including Linux -> Linux CMake")
    include(CMakeLists-Linux.txt)
elseif(TARGET_PLATFORM STREQUAL "LINUX_WIN")
    message(STATUS "Including Linux -> Windows CMake")
    include(CMakeLists-Linux_to_Windows.txt)
elseif(TARGET_PLATFORM STREQUAL "WIN")
    message(STATUS "Including Windows -> Windows CMake")
    include(CMakeLists-NativeWindows.txt)
else()
    message(FATAL_ERROR "Unknown TARGET_PLATFORM=${TARGET_PLATFORM}")
endif()