# Standard headers (precompiled)
set(STANDARD_LIBRARY_HEADERS
    "<iostream>"
    "<vector>"
    "<array>"
    "<unordered_map>"
    "<string>"
    "<filesystem>"
    "<chrono>"
    "<future>"
    "<queue>"
    "<functional>"
    "<optional>"
    "<utility>"
    "<fstream>"
    "<sstream>"
    "<cstdint>"
    "<cstring>"
    "<cstddef>"
    "<cstdio>"
    "<cstdlib>"
    "<csignal>"
    "<map>"
    "<thread>"
    "<mutex>"
    "<memory>"
    "<algorithm>"
)

add_library(rat_precompiled_headers INTERFACE)

# Third-party include paths
find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)
find_package(fmt REQUIRED)

target_include_directories(rat_precompiled_headers INTERFACE
    ${ZLIB_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${FMT_INCLUDE_DIRS}
)

# Precompile only standard headers and your own headers (not zlib, curl, etc.)
if(WIN32)
    target_precompile_headers(rat_precompiled_headers INTERFACE
        ${STANDARD_LIBRARY_HEADERS}
        "windows.h"
    )
else()
    target_precompile_headers(rat_precompiled_headers INTERFACE
        ${STANDARD_LIBRARY_HEADERS}
    )
endif()
