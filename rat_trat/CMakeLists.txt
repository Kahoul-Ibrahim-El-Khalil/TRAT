set(SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

add_executable(trat ${SRCS})

target_include_directories(trat
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(DEFINED SPDLOG_HEADERS)
    target_include_directories(trat PRIVATE ${SPDLOG_HEADERS})
endif()

target_link_libraries(trat
    PRIVATE
        rat_handler
)



if(DEBUG)
    target_link_libraries(trat PRIVATE spdlog::spdlog)
    target_compile_definitions(trat PRIVATE DEBUG)
endif()
