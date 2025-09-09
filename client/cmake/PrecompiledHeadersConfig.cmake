set(STANDARD_LIBRARY_HEADERS
    <iostream>
    <vector>
    <array>
    <string>
    <filesystem>
    <chrono>
    <future>
    <queue>
    <functional>
    <optional>
	<fstream>
    <sstream>
    <cstdint>
    <cstring>
    <cstddef>
    <cstdio>
    <cstdlib>
    <csignal>
    <map>
    <thread>
    <mutex>
    <memory>
    <algorithm>
)
set(THIRD_PARTY_HEADERS
    <curl/curl.h>
    <fmt/core.h>
    <fmt/chrono.h>
    <simdjson.h>
    <tiny-process-library/process.hpp>
    <boost/algorithm/string.hpp>
)
if(DEBUG)
    find_package(spdlog REQUIRED)
    list(APPEND THIRD_PARTY_HEADERS <spdlog/spdlog.h>)
endif()

add_library(rat_precompiled_headers INTERFACE)

if(WIN32)
    set(WINDOWS_API_HEADERS <windows.h>)
    target_precompile_headers(rat_precompiled_headers INTERFACE
        ${STANDARD_LIBRARY_HEADERS}
        ${THIRD_PARTY_HEADERS}
        ${WINDOWS_API_HEADERS}
    )
elseif(UNIX)
    target_precompile_headers(rat_precompiled_headers INTERFACE
        ${STANDARD_LIBRARY_HEADERS}
        ${THIRD_PARTY_HEADERS}
    )
endif()

