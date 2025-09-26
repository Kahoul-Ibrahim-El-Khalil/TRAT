set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(TBOT_SOURCES ${SRC_DIR}/BaseBot.cpp
                 ${SRC_DIR}/Bot.cpp
)

add_library(rat_tbot STATIC ${TBOT_SOURCES})

target_include_directories(rat_tbot
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
      )

target_link_libraries(rat_tbot
    PUBLIC
		simdjson
        rat_networking        # internal dependency
        fmt              # from root
)

if (DEBUG_RAT_TBOT)
	target_compile_definitions(rat_tbot PUBLIC DEBUG_RAT_TBOT)
endif()

if(BUILD_DUMMY_BOT)
	add_executable(DummyBot src/DummyBot.cpp)
endif()
