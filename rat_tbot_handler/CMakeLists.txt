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
    ${SRC_DIR}/handleUpdate.cpp
    ${SRC_DIR}/handleUpdateCommandParser.cpp
    ${SRC_DIR}/handleMediaCommands.cpp
)

# ======================================
# Library target (controlled by root: BUILD_STATIC)
# ======================================
add_library(rat_tbot_handler STATIC ${HANDLER_SRC})

target_include_directories(rat_tbot_handler
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ======================================
# Dependencies (all injected from root)
# ======================================
target_link_libraries(rat_tbot_handler
    PUBLIC
        rat_state
        rat_networking
        rat_tbot
        rat_system
        rat_media
)

# Windows extras
if(WIN32)
    target_link_libraries(rat_tbot_handler PUBLIC ws2_32 wldap32 crypt32 normaliz)
else()
    target_link_libraries(rat_tbot_handler PUBLIC pthread)
endif()

# ======================================
# Debug logging (root DEBUG flag)
# ======================================
if(DEBUG)
    target_link_libraries(rat_tbot_handler PUBLIC spdlog::spdlog)
    target_compile_definitions(rat_tbot_handler PUBLIC DEBUG)
endif()

# ======================================
# Example executable (optional, for standalone builds)
# ======================================

