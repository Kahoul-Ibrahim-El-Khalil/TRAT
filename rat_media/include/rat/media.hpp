#pragma once

#include <filesystem>
#include <string>
#include <cstdint>
#include <optional>
#include <atomic>
#include <algorithm>  // ← ADD THIS for std::find
#include <fmt/core.h>
#include <vector>
#include "rat/media.hpp"
namespace rat::media::screenshot {

struct Resolution;

struct ImageConfig;

bool is_ffmpeg_available();

std::string get_ffmpeg_version();



// Screenshot functions
bool takeScreenshot(
    const std::filesystem::path& Output_Path,
    std::string& Output_Buffer
);

} // namespace rat::media::screenshot
