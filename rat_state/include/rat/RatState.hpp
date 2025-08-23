#pragma once
#include <unordered_map>
#include <string>
#include <filesystem>


#pragma once
#include <unordered_map>
#include <string>
#include <filesystem>
#include <vector>
#include <cstdlib>
#include <optional>

namespace rat {

enum class HostOperatingSystem {
    WINDOWS,
    LINUX
};

struct RatState {

    // -------------------
    // Host / Environment Info
    // -------------------
    HostOperatingSystem host_system;                    // Current host OS
    std::vector<std::filesystem::path> custom_tool_dirs;
    // -------------------
    // Maps
    // -------------------
    std::unordered_map<std::string, std::string>    system_globals;   // OS-level or tool-level globals
    /*
    The hostname, the system drive,..
    */
    std::unordered_map<std::string, std::filesystem::path> system_utils;  // Dynamic tools (ffmpeg, curl, etc.)
    std::unordered_map<std::string, std::string> dynamic_commands;        // User-defined commands
    std::unordered_map<std::string, std::string> dynamic_globals;         // User-defined globals

    // -------------------
    // Constructor
    // -------------------
    RatState();

    // -------------------
    // System utilities
    // -------------------
    void scanSystemPathsForUtilities();                            // Scan system PATH for executables
    void scanDirectoryForUtilities(const std::filesystem::path& dir, bool prioritize = false); // Scan a directory
    void detectCommonUtilities();                                   // Preload common tools: ffmpeg, curl, wget
    void addCustomToolDir(const std::filesystem::path& dir);       // Add a custom directory and scan
    std::filesystem::path getUtilityPath(const std::string& arg_Name) const;

    // -------------------
    // Dynamic commands
    // -------------------
    void addCommand(const std::string& Command_Key, const std::string& Command_Literal);
    std::string getCommand(const std::string& Command_Key) const;
    void resetCommands();                                          // Clear all dynamic commands

    // -------------------
    // Dynamic globals
    // -------------------
    void setGlobal(const std::string& arg_Key, const std::string& arg_Value);
    std::string getGlobal(const std::string& arg_Key) const;
    void resetGlobals();                                           // Clear all dynamic globals

    // -------------------
    // Utility / Helper methods
    // -------------------
    std::vector<std::string> listDynamicTools() const;             // Return all keys from system_utils
    std::vector<std::string> listDynamicCommands() const;          // Return all dynamic command keys
    std::vector<std::string> listDynamicGlobals() const;           // Return all dynamic global keys
    std::vector<std::string> listSystemGlobals() const;            // Return all system globals

    bool hasUtility(const std::string& tool_name) const;           // Check if a tool is available
    bool hasCommand(const std::string& cmd_name) const;            // Check if a dynamic command exists
    bool hasGlobal(const std::string& key_name) const;             // Check if a dynamic global exists
};

} // namespace rat
