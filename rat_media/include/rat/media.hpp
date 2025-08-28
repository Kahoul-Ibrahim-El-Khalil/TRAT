#pragma once

#include <filesystem>
#include <string>
#include <cstdint>
#include <optional>
#include <atomic>
#include <algorithm>  // ← ADD THIS for std::find
#include <fmt/core.h>
#include <vector>

namespace rat::media::screenshot {

struct Resolution {
    uint16_t width;
    uint16_t height;

    Resolution(uint16_t arg_Width, uint16_t arg_Height)
      :
        width(arg_Width),
        height(arg_Height){};
    std::string toString() const {
        return fmt::format("{}x{}", width, height);
    }
    double getRatio() const {
        return static_cast<double>(width) / static_cast<double>(height);
    }
};

struct ImageConfig;

bool is_ffmpeg_available();

std::string get_ffmpeg_version();



// Screenshot functions
bool takeScreenshot(
    const std::filesystem::path& Output_Path,
    std::string& Output_Buffer
);

} // namespace rat::media::screenshot
