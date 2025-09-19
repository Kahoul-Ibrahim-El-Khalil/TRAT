
cmake_minimum_required(VERSION 3.25)
project(DrogonRatServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Drogon
find_package(Drogon REQUIRED)

# Find OpenSSL
find_package(OpenSSL REQUIRED)

# Include your headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# Sources
set(SRCS src/main.cpp
         src/Server/methods.cpp
         src/Handler/methods.cpp
)

# Executable
add_executable(server ${SRCS})

# Link Drogon + OpenSSL
target_link_libraries(server
    PRIVATE
        Drogon::Drogon
        OpenSSL::SSL
        OpenSSL::Crypto
)

