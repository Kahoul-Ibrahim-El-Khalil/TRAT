# Library target
add_library(rat_threading
    src/ThreadPool.cpp
)

target_include_directories(rat_threading
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(rat_threading PUBLIC rat_precompiled_headers)

if (DEBUG_RAT_THREADING)
    target_compile_definitions(rat_threading PUBLIC DEBUG_RAT_THREADING)
    target_link_libraries(rat_threading PUBLIC spdlog::spdlog)
endif()
