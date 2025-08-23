set(SRCS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SRCS ${SRCS_DIR}/xor.cpp)

# Respect global BUILD_STATIC (from root CMakeLists.txt)
add_library(rat_encryption STATIC ${SRCS})

target_include_directories(rat_encryption
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
