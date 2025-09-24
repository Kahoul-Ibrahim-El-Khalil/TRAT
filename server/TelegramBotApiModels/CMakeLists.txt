cmake_minimum_required(VERSION 3.15)
project(TelegramBotApiModels LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Collect generated model sources
file(GLOB_RECURSE DB_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# Build a static library (you can change to SHARED if you want .so/.dll)
add_library(TelegramBotApiModels STATIC ${DB_SOURCES})

# Set namespace in IDE (optional, but nice)
set_target_properties(TelegramBotApiModels PROPERTIES
    OUTPUT_NAME "TelegramBotApiModels"
)

# Link against Drogon (for ORM + db access)
target_link_libraries(TelegramBotApiModels
    PUBLIC Drogon::Drogon
)

# Expose headers (models) to parent projects
target_include_directories(TelegramBotApiModels
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

