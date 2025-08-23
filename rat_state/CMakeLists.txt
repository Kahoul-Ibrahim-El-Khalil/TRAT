set(SRCS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SRCS ${SRCS_DIR}/RatState.cpp)

# Create static library
add_library(rat_state STATIC ${SRCS})

target_include_directories(rat_state
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Create test executable
add_executable(rat_state_test ${SRCS_DIR}/test.cpp)

# Link test executable to library
target_link_libraries(rat_state_test PUBLIC rat_state)
