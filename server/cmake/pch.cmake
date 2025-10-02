add_library(PCH INTERFACE)
target_precompile_headers(PCH INTERFACE
    <drogon/HttpAppFramework.h>
    <drogon/HttpResponse.h>
    <drogon/HttpRequest.h>
    <drogon/drogon.h>
    <drogon/orm/DbClient.h>
    <drogon/orm/Mapper.h>
    <json/value.h>
    <functional>
    <fmt/core.h>
    <filesystem>
    <algorithm>
    <string>
    <vector>
    <fstream>
    <string_view>
    <utility>
    <chrono>
    <random>
    <mutex>
)

