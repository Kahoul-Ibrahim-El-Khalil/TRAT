# rat_trat/CMakeLists.txt
cmake_minimum_required(VERSION 3.25)

# ======================================
# Sources
# ======================================
set(SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

# ======================================
# Executable target
# ======================================
add_executable(trat ${SRCS})

target_include_directories(trat
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Allow root to inject spdlog headers if needed
if(DEFINED SPDLOG_HEADERS)
    target_include_directories(trat PRIVATE ${SPDLOG_HEADERS})
endif()

# ======================================
# Internal dependencies
# ======================================
target_link_libraries(trat
    PRIVATE
        rat_process
        rat_encryption
        rat_networking
        rat_system
        rat_tbot
        rat_handler
)



# ======================================
# Debug support (root DEBUG flag)
# ======================================
if(DEBUG)
    target_link_libraries(trat PRIVATE spdlog::spdlog)
    target_compile_definitions(trat PRIVATE DEBUG)
endif()
