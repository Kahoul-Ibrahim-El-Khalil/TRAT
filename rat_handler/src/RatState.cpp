#include "rat/handler/types.hpp"  // Make sure we include the declaration
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <fmt/core.h>
#include <unordered_map>

#ifdef _WIN32
#include <windows.h>
#else
#include <cstdio>    // popen, fgets, pclose
#include <unistd.h>
#endif

#include <filesystem>
#include <sstream>
#include <cstdio>

namespace rat::handler {

// -------------------
// Tool lists
// -------------------
#ifdef _WIN32
static constexpr const char* TOOLS[] = {
    // External binaries
    "ffmpeg", "curl", "wget", "7z", "zip", "tar", "python", "node", "git",

    // Native commands / utilities
    "powershell", "cmd", "tasklist", "taskkill", "reg", "schtasks",
    "netstat", "ipconfig", "wmic", "msinfo32", "sc",
    "ping", "tracert", "net",

    // Common applications
    "explorer", "notepad", "calc", "mspaint"
};
#else
static constexpr const char* TOOLS[] = {
    // External binaries
    "ffmpeg", "curl", "wget", "7z", "zip", "tar", "python", "node", "git",

    // Common Linux commands
    "bash", "sh", "ls", "ps", "kill", "top", "htop",
    "df", "du", "ifconfig", "ip", "ping", "traceroute",
    "netstat", "ss", "cron", "systemctl"
};
#endif
constexpr size_t KB = 1024;

// -------------------
// Constructor
// -------------------
RatState::RatState() {
    for (size_t i = 0; i < std::size(TOOLS); ++i) {
        auto path = findExecutable(TOOLS[i]);
        if (!path.empty()) {
            command_path_map[TOOLS[i]] = path;
        }
    }
    this->payload.reserve(50*KB);
}

// Tool path resolver
std::filesystem::path RatState::findExecutable(const std::string& arg_Tool) const {
#ifdef _WIN32
    char buffer[MAX_PATH];
    if (SearchPathA(NULL, arg_Tool.c_str(), ".exe", MAX_PATH, buffer, NULL) != 0) {
        return std::filesystem::path(buffer);
    }
    return {};
#else
    std::string cmd = fmt::format("which {} 2>/dev/null", arg_Tool); 
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return {};

    char buffer[512];
    if (fgets(buffer, sizeof(buffer), pipe)) {
        std::string result(buffer);
        result.erase(result.find_last_not_of(" \n\r\t") + 1); // trim newline
        pclose(pipe);
        return std::filesystem::path(result);
    }
    pclose(pipe);
    return {};
#endif
}

// -------------------
// API
// -------------------
std::filesystem::path RatState::getToolPath(const std::string& arg_Name) const {
    auto it = command_path_map.find(arg_Name);
    if (it != command_path_map.end()) return it->second;
    return {};
}

bool RatState::hasTool(const std::string& arg_Tool) const {
    return command_path_map.find(arg_Tool) != command_path_map.end();
}

void RatState::addCommand(const std::string& arg_Key, const std::string& arg_Literal) {
    this->command_path_map[arg_Key] = arg_Literal;
}

std::string RatState::getCommand(const std::string& arg_Key) const {
    auto it = this->command_path_map.find(arg_Key);
    if (it != this->command_path_map.end()) return it->second.string();
    return {};
}

std::string RatState::listDynamicTools() const {
    std::ostringstream output_string_stream;

    for (const auto& [command, path] : command_path_map) {
        output_string_stream << fmt::format("{}: {}\n", command, path.string());
    }
    return output_string_stream.str();
}
} // namespace rat::handler
