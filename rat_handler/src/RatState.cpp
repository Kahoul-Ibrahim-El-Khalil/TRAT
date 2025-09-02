#include "rat/handler/types.hpp"  // Make sure we include the declaration
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <fmt/core.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <cstdio>    // popen, fgets, pclose
#include <unistd.h>
#endif

#include <filesystem>
#include <sstream>
#include <cstdio>

#include "rat/handler/debug.hpp"

namespace rat::handler {

#ifdef _WIN32
static constexpr const char* TOOLS[] = {
    "ffmpeg", "curl", "wget", "7z", "zip", "tar", "python", "node", "git",
    "powershell", "cmd", "tasklist", "taskkill", "reg", "schtasks",
    "netstat", "ipconfig", "wmic", "msinfo32", "sc",
    "ping", "tracert", "net",
    "explorer", "notepad", "calc", "mspaint"
};
#else
static constexpr const char* TOOLS[] = {
    "ffmpeg", "curl", "wget", "7z", "zip", "tar", "python", "node", "git",
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
    for (size_t i = 0; i < ::std::size(TOOLS); ++i) {
        auto path = findExecutable(TOOLS[i]);
        if (!path.empty()) {
            this->command_path_map.push_back(TOOLS[i], ::std::move(path));
        }
    }
    this->payload.reserve(50 * KB);
}

// -------------------
// Tool path resolver
// -------------------
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
        result.erase(result.find_last_not_of(" \n\r\t") + 1);
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
    if (auto* entry = command_path_map.find(arg_Name)) {
        return entry->path;
    }
    return {};
}

bool RatState::hasTool(const std::string& arg_Tool) const {
    return command_path_map.find(arg_Tool) != nullptr;
}

void RatState::addCommand(const std::string& arg_Key, const std::string& arg_Literal) {
    if (auto* entry = command_path_map.find(arg_Key)) {
        entry->path = arg_Literal; // overwrite
    } else {
        command_path_map.push_back(arg_Key, std::filesystem::path(arg_Literal));
    }
}

std::string RatState::getCommand(const std::string& arg_Key) const {
    if (auto* entry = command_path_map.find(arg_Key)) {
        return entry->path.string();
    }
    return {};
}

std::string RatState::listDynamicTools() const {
    std::ostringstream output_string_stream;
    for (const auto& entry : command_path_map) {
        output_string_stream << fmt::format("{}: {}\n", entry.command, entry.path.string());
    }
    return output_string_stream.str();
}

} // namespace rat::handler

#undef HANDLER_DEBUG_LOG
#undef HANDLER_ERROR_LOG
#ifdef DEBUG_RAT_HANDLER
    #undef DEBUG_RAT_HANDLER
#endif
