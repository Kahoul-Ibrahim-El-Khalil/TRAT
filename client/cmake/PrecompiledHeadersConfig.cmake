set(STANDARD_LIBRARY_HEADERS
    <iostream>
    <vector>
    <array>
    <unordered_map>
	<string>
    <filesystem>
    <chrono>
    <future>
    <queue>
    <functional>
    <optional>
    <utility>
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
    <zlib.h>
	<zstd.h>
	<curl/curl.h>
    <fmt/core.h>
    <fmt/chrono.h>
	<fmt/format.h>
    <simdjson.h>
    <tiny-process-library/process.hpp>
    <boost/algorithm/string.hpp>
)


if(DEBUG)
    find_package(spdlog REQUIRED)
    list(APPEND  <spdlog/spdlog.h>
				 <spdlog/sinks/stdout_color_sinks.h>
	)
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

