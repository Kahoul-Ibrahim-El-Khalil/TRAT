
add_library(rat_networking STATIC
                ${CMAKE_CURRENT_SOURCE_DIR}/src/EasyCurlHandler.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/src/Client.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/src/helpers.cpp )

target_include_directories(rat_networking
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(rat_networking
    PUBLIC
		CURL::libcurl
)


add_executable(rat_networker src/networker.cpp)
target_link_libraries(rat_networker PRIVATE rat_networking)
