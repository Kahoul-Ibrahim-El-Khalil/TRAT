# rat_tbot/CMakeLists.txt
# ======================================
# Sources
# ======================================
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(TBOT_SOURCES ${SRC_DIR}/tbot.cpp)

# ======================================
# Library target (controlled by root: BUILD_STATIC)
# ======================================
add_library(rat_tbot STATIC ${TBOT_SOURCES})

target_include_directories(rat_tbot
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ======================================
# Dependencies (all injected by root)
# ======================================
target_link_libraries(rat_tbot
    PUBLIC
        rat_networking        # internal dependency
        fmt              # from root
)



# Debug logging with spdlog (controlled by root DEBUG flag)
if(DEBUG)
    target_link_libraries(rat_tbot PUBLIC spdlog::spdlog)
    target_compile_definitions(rat_tbot PUBLIC DEBUG)
endif()

# ======================================
# Example executable (for standalone builds only)
# ======================================
add_executable(test_bot src/main.cpp)
target_link_libraries(test_bot PRIVATE rat_tbot)

