set(SRCS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SRCS ${SRCS_DIR}/zlibCompression.cpp)


add_library(rat_compression STATIC ${SRCS})

target_include_directories(rat_compression
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(rat_compression PUBLIC zlib)
