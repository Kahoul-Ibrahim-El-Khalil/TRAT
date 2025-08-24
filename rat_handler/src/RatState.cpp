#include "rat/handler/types.hpp"
#include <filesystem>
#include <cstdlib>
#include <algorithm>
#include <iostream>

namespace rat::handler{

// -------------------
// Constructor
// -------------------
RatState::RatState() {
#ifdef _WIN32
    host_system = HostOperatingSystem::WINDOWS;
#else
    host_system = HostOperatingSystem::LINUX;
#endif
}

// -------------------
// System Utilities
// -------------------
void RatState::scanSystemPathsForUtilities() {
    const char* path_env = std::getenv("PATH");
    if (!path_env) return;

    std::string paths(path_env);
    std::string delimiter =
#ifdef _WIN32
        ";";
#else
        ":";
#endif

    size_t start = 0;
    size_t end = paths.find(delimiter);
    while (end != std::string::npos) {
        scanDirectoryForUtilities(paths.substr(start, end - start));
        start = end + delimiter.length();
        end = paths.find(delimiter, start);
    }
    scanDirectoryForUtilities(paths.substr(start));
}

void RatState::scanDirectoryForUtilities(const std::filesystem::path& dir, bool prioritize) {
    if (!std::filesystem::exists(dir)) return;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;

        std::filesystem::path path = entry.path();
        std::string name = path.stem().string();
#ifdef _WIN32
        std::string ext = path.extension().string();
        if (ext != ".exe" && ext != ".bat") continue;
#endif
        if (prioritize || system_utils.find(name) == system_utils.end()) {
            system_utils[name] = path;
        }
    }
}

void RatState::detectCommonUtilities() {
    std::vector<std::string> commonTools = {"ffmpeg", "curl", "wget"};
    for (const auto& tool : commonTools) {
        if (getUtilityPath(tool).empty()) {
            // Try to find it in PATH
            scanSystemPathsForUtilities();
        }
    }
}

void RatState::addCustomToolDir(const std::filesystem::path& dir) {
    custom_tool_dirs.push_back(dir);
    scanDirectoryForUtilities(dir, true);
}

std::filesystem::path RatState::getUtilityPath(const std::string& arg_Name) const {
    auto it = system_utils.find(arg_Name);
    if (it != system_utils.end()) return it->second;
    return {};
}

bool RatState::hasUtility(const std::string& tool_name) const {
    return system_utils.find(tool_name) != system_utils.end();
}

std::vector<std::string> RatState::listDynamicTools() const {
    std::vector<std::string> keys;
    for (const auto& [k, v] : system_utils) keys.push_back(k);
    return keys;
}

// -------------------
// Dynamic Commands
// -------------------
void RatState::addCommand(const std::string& Command_Key, const std::string& Command_Literal) {
    dynamic_commands[Command_Key] = Command_Literal;
}

std::string RatState::getCommand(const std::string& Command_Key) const {
    auto it = dynamic_commands.find(Command_Key);
    if (it != dynamic_commands.end()) return it->second;
    return {};
}

void RatState::resetCommands() {
    dynamic_commands.clear();
}

bool RatState::hasCommand(const std::string& cmd_name) const {
    return dynamic_commands.find(cmd_name) != dynamic_commands.end();
}

std::vector<std::string> RatState::listDynamicCommands() const {
    std::vector<std::string> keys;
    for (const auto& [k, v] : dynamic_commands) keys.push_back(k);
    return keys;
}

// -------------------
// Dynamic Globals
// -------------------
void RatState::setGlobal(const std::string& arg_Key, const std::string& arg_Value) {
    dynamic_globals[arg_Key] = arg_Value;
}

std::string RatState::getGlobal(const std::string& arg_Key) const {
    auto it = dynamic_globals.find(arg_Key);
    if (it != dynamic_globals.end()) return it->second;
    return {};
}

void RatState::resetGlobals() {
    dynamic_globals.clear();
}

bool RatState::hasGlobal(const std::string& key_name) const {
    return dynamic_globals.find(key_name) != dynamic_globals.end();
}

std::vector<std::string> RatState::listDynamicGlobals() const {
    std::vector<std::string> keys;
    for (const auto& [k, v] : dynamic_globals) keys.push_back(k);
    return keys;
}

// -------------------
// System Globals
// -------------------
std::vector<std::string> RatState::listSystemGlobals() const {
    std::vector<std::string> keys;
    for (const auto& [k, v] : system_globals) keys.push_back(k);
    return keys;
}

} // namespace rat
