
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(HANDLER_SRC
    ${SRC_DIR}/media.cpp
)

add_library(rat_media STATIC ${HANDLER_SRC})

target_include_directories(rat_media
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(rat_media
    PUBLIC
		fmt
        rat_system
)

if(DEBUG)
    target_link_libraries(rat_media PUBLIC spdlog::spdlog)
    target_compile_definitions(rat_media PUBLIC DEBUG)
endif()

add_executable(media ${CMAKE_CURRENT_SOURCE_DIR}/src/test.cpp)

target_link_libraries(media PUBLIC rat_media rat_system)
