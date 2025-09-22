cmake_minimum_required(VERSION 3.15)
project(DrogonRatDatabase LANGUAGES CXX)

# Require C++17 (needed by Drogon ORM)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Collect generated model sources
file(GLOB_RECURSE DB_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# Build a static library (you can change to SHARED if you want .so/.dll)
add_library(DrogonRatDatabase STATIC ${DB_SOURCES})

# Set namespace in IDE (optional, but nice)
set_target_properties(DrogonRatDatabase PROPERTIES
    OUTPUT_NAME "DrogonRatDatabase"
)

# Link against Drogon (for ORM + db access)
target_link_libraries(DrogonRatDatabase
    PUBLIC Drogon::Drogon
)

# Expose headers (models) to parent projects
target_include_directories(DrogonRatDatabase
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

