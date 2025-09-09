set(SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)
add_executable(trat ${SRCS})
target_include_directories(trat
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(trat
    PRIVATE
        rat_precompiled_headers
		rat_handler
)
