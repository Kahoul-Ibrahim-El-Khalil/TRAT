# rat_tbot_handler/CMakeLists.txt
cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ======================================
# Sources
# ======================================
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(HANDLER_SRC
    ${SRC_DIR}/media.cpp
)

# ======================================
# Library target (controlled by root: BUILD_STATIC)
# ======================================
add_library(rat_media STATIC ${HANDLER_SRC})

target_include_directories(rat_media
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ======================================
# Dependencies (all injected from root)
# ======================================
target_link_libraries(rat_media
    PUBLIC
        fmt
)

# ======================================
# Debug logging (root DEBUG flag)
# ======================================
if(DEBUG)
    target_link_libraries(rat_media PUBLIC spdlog::spdlog)
    target_compile_definitions(rat_media PUBLIC DEBUG)
endif()

add_executable(media ${CMAKE_CURRENT_SOURCE_DIR}/src/test.cpp)

target_link_libraries(media PUBLIC rat_media rat_system)
# ======================================
# Example executable (optional, for standalone builds)
# ======================================

