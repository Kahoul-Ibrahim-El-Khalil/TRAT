# rat_system/CMakeLists.txt
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(WIN32)
    set(SRC_INFO_MODULE ${SRC_DIR}/info/win_info.cpp)
elseif(APPLE)
    set(SRC_INFO_MODULE ${SRC_DIR}/info/mac_info.cpp)
else()
    set(SRC_INFO_MODULE ${SRC_DIR}/info/linux_info.cpp)
endif()

set(SYSTEM_SRC
    ${SRC_DIR}/filesystem/path_normalization.cpp
    ${SRC_DIR}/filesystem/directory_operations.cpp
    ${SRC_DIR}/filesystem/file_operations.cpp
    ${SRC_DIR}/filesystem/navigation.cpp
    ${SRC_DIR}/filesystem/queries.cpp
    ${SRC_DIR}/time.cpp
    ${SRC_DIR}/privilege.cpp
    ${SRC_DIR}/rawdogger/rawdogger.cpp
    ${SRC_INFO_MODULE}
)
add_library(rat_system STATIC ${SYSTEM_SRC})

target_include_directories(rat_system
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include

      )

target_link_libraries(rat_system PUBLIC fmt stdc++fs rat_threading)

if (DEBUG_RAT_SYSTEM)
	target_compile_definitions(rat_system PUBLIC DEBUG_RAT_SYSTEM)
endif()

#add_executable(rat_shell src/shell.cpp)
#add_executable(rawdogger_test src/rawdogger/test/main.cpp)

#target_link_libraries(rat_shell PRIVATE rat_system)
#target_link_libraries(rawdogger_test PRIVATE rat_system)

