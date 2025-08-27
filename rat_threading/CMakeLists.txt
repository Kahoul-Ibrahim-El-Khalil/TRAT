# Library target
add_library(rat_threading
    src/ThreadPool.cpp
)

target_include_directories(rat_threading
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

