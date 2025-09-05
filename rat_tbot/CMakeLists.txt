set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(TBOT_SOURCES ${SRC_DIR}/BaseBot.cpp
                 ${SRC_DIR}/Bot.cpp
)

add_library(rat_tbot STATIC ${TBOT_SOURCES})

target_include_directories(rat_tbot
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/simdjson
      )

target_link_libraries(rat_tbot
    PUBLIC
		libsimdjson
        rat_networking        # internal dependency
        fmt              # from root
)



if(DEBUG)
    target_link_libraries(rat_tbot PUBLIC spdlog::spdlog)
    target_compile_definitions(rat_tbot PUBLIC DEBUG)
endif()

add_executable(test_bot src/main.cpp)
target_link_libraries(test_bot PRIVATE rat_tbot)

