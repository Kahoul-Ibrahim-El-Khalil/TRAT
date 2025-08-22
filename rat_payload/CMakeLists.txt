cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# === Options ===
option(DEBUG "Enable debug logging" OFF)
option(SHARED "Build rat_payload as a shared library" OFF)

# === Installation/dependencies Paths ===
set(LOCAL_PROJECT_DIR ${CMAKE_SOURCE_DIR}/..)
set(LOCAL_INCLUDE_DIR ${LOCAL_PROJECT_DIR}/include)
set(LOCAL_LIB_DIR ${LOCAL_PROJECT_DIR}/lib)

set(RAT_PAYLOAD_SOURCES
    src/PayloadManager.cpp
    src/rawdogger.cpp
)

# === Library target ===
add_library(rat_payload STATIC ${RAT_PAYLOAD_SOURCES})

target_include_directories(rat_payload PUBLIC 
  ${LOCAL_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include      
)

# === Dependencies ===
find_package(fmt REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem system)

# Add spdlog headers if debug logs enabled
if(DEBUG)
    if(NOT DEFINED SPDLOG_INCLUDE_DIR)
        set(SPDLOG_INCLUDE_DIR "/usr/include") # Linux default
    endif()
    target_include_directories(rat_payload PUBLIC ${SPDLOG_INCLUDE_DIR})
endif()

target_link_libraries(rat_payload PUBLIC 
    fmt::fmt
    ${LOCAL_LIB_DIR}/librat_system.a
    ${LOCAL_LIB_DIR}/librat_encryption.a
    Boost::filesystem
    Boost::system
)

add_executable(rat_payload_test  src/test.cpp)
target_link_libraries(rat_payload_test PUBLIC 
    rat_payload
    fmt::fmt
    ${LOCAL_LIB_DIR}/librat_system.a
    ${LOCAL_LIB_DIR}/librat_encryption.a
    Boost::filesystem
    Boost::system
)
