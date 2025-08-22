cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRCS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SRCS ${SRCS_DIR}/xor.cpp)

# Respect global BUILD_STATIC (from root CMakeLists.txt)
add_library(rat_encryption STATIC ${SRCS})

target_include_directories(rat_encryption
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
