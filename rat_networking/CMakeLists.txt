# rat_networking/CMakeLists.txt
cmake_minimum_required(VERSION 3.25)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------
# Library (respects BUILD_STATIC from root)
# ---------------------------------------
add_library(rat_networking STATIC src/networking.cpp)

target_include_directories(rat_networking
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(rat_networking PRIVATE -Oz)

# ---------------------------------------
# Dependencies (provided from root)
# ---------------------------------------
target_link_libraries(rat_networking
    PUBLIC
        CURL::libcurl
)

# ---------------------------------------
# Example executable (for standalone builds only)
# ---------------------------------------

add_executable(rat_networker src/networker.cpp)
target_link_libraries(rat_networker PRIVATE rat_networking)
