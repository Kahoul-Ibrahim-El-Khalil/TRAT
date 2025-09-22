cmake_minimum_required(VERSION 3.25)
project(DrogonRatServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Option for debug logging
option(DEBUG "Enable debug logging with spdlog" OFF)

# Find Drogon
find_package(Drogon REQUIRED)

# Find OpenSSL
find_package(OpenSSL REQUIRED)

# Database submodule
add_subdirectory(database)

# Include your headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# Sources
set(SRCS
    src/main.cpp
    src/Server/methods.cpp
    src/Handler/methods.cpp
    src/Handler/constructor.cpp
)

# Executable
add_executable(server ${SRCS})

# Link Drogon + OpenSSL + DB module
target_link_libraries(server
    PRIVATE
        Drogon::Drogon
        OpenSSL::SSL
        OpenSSL::Crypto
        DrogonRatDatabase
)


# On Windows/MinGW: copy OpenSSL DLLs into output dir
if(WIN32)
    # Explicit path to MSYS2 OpenSSL DLLs
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
