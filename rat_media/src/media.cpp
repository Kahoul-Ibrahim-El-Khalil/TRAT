#include "rat/media.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace rat::media::screenshot {

// --- Check ffmpeg availability ---
bool is_ffmpeg_available() {
#ifdef _WIN32
	FILE *pipe = _popen("where ffmpeg", "r");
#else
	FILE *pipe = popen("command -v ffmpeg", "r");
#endif
	if(!pipe)
		return false;

	char buffer[128];
	bool available = false;
		if(fgets(buffer, sizeof(buffer), pipe)) {
			available = true;
		}

#ifdef _WIN32
	_pclose(pipe);
#else
	pclose(pipe);
#endif
	return available;
}

// --- Get ffmpeg version ---
std::string get_ffmpeg_version() {
#ifdef _WIN32
	FILE *pipe = _popen("ffmpeg -version", "r");
#else
	FILE *pipe = popen("ffmpeg -version", "r");
#endif
	if(!pipe)
		return "unknown";

	char buffer[256];
	std::string version;
		if(fgets(buffer, sizeof(buffer), pipe)) {
			version = buffer;
		}

#ifdef _WIN32
	_pclose(pipe);
#else
	pclose(pipe);
#endif
	return version;
}

// --- Display info ---
Resolution get_display_resolution(uint8_t = 0) {
#ifdef _WIN32
	return {static_cast<uint16_t>(GetSystemMetrics(SM_CXSCREEN)),
	        static_cast<uint16_t>(GetSystemMetrics(SM_CYSCREEN))};
#else
	return {1920, 1080}; // fallback, ffmpeg handles actual capture
#endif
}

uint8_t get_display_count() {
#ifdef _WIN32
	return static_cast<uint8_t>(GetSystemMetrics(SM_CMONITORS));
#else
	return 1;
#endif
}

// --- Take screenshot synchronously ---
bool takeScreenshot(const std::filesystem::path &outputPath,
                    std::string &outputBuffer) {
		if(outputPath.empty()) {
			outputBuffer = "Output path is empty";
			return false;
		}

		if(!is_ffmpeg_available()) {
			outputBuffer = "ffmpeg not available in PATH";
			return false;
		}

	std::string cmd;
#ifdef _WIN32
	Resolution res = get_display_resolution();
	cmd = "ffmpeg -y -f gdigrab -framerate 1 -video_size " + res.toString() +
	      " -i desktop -frames:v 1 \"" + outputPath.string() + "\" >nul 2>&1";
#else
	cmd = "ffmpeg -y -f x11grab -i :0.0 -frames:v 1 \"" + outputPath.string() +
	      "\" >/dev/null 2>&1";
#endif

	int ret = std::system(cmd.c_str());
		if(ret != 0 || !std::filesystem::exists(outputPath)) {
			outputBuffer = "ffmpeg failed to capture screenshot";
			return false;
		}

	return true;
}

} // namespace rat::media::screenshot
