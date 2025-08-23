# rat_system/CMakeLists.txt
cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ======================================
# Sources
# ======================================
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
    ${SRC_DIR}/os-process.cpp
    ${SRC_DIR}/process.cpp
    ${SRC_DIR}/time.cpp
    ${SRC_DIR}/privilege.cpp
    ${SRC_DIR}/rawdogger/rawdogger.cpp
    ${SRC_INFO_MODULE}
)
# A new Dependency Tiny-Process
# ======================================
# Library target (controlled by root: BUILD_STATIC)
# ======================================
add_library(rat_system STATIC ${SYSTEM_SRC})

target_include_directories(rat_system
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include

      )

# ======================================
# Dependencies (injected from root)
# ======================================
# Tiny-process
target_link_libraries(rat_system PUBLIC fmt Threads::Threads tiny-process)

# pthread on UNIX
if(UNIX AND NOT APPLE)
    target_link_libraries(rat_system PUBLIC pthread)
endif()

# spdlog (if DEBUG=ON from root)
if(DEBUG)
    target_link_libraries(rat_system PUBLIC spdlog::spdlog)
    target_compile_definitions(rat_system PUBLIC DEBUG)
endif()

# ======================================
# Debug shell executable (for standalone builds only)
# ======================================
add_executable(rat_shell src/shell.cpp)
add_executable(rawdogger_test src/rawdogger/test/main.cpp)

target_link_libraries(rat_shell PRIVATE rat_system)
target_link_libraries(rawdogger_test PRIVATE rat_system)

