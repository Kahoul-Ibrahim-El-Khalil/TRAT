#pragma once
/*
 * rat::system
 * Cross-platform system operation API.
 *
 * Each function takes simple arguments and returns either:
 *   - std::string  → textual data or serialized output
 *   - bool         → success/failure
 *   - size_t       → file/dir sizes
 *
 * Arguments use consistent naming: arg_X for values, Dir_Path/File_Path for filesystem paths.
 */
#include <iostream>
#include <filesystem>
#include <string>
#include <chrono>
#include <vector>
#include <thread>
#include <functional>
#include <cstdint>
#include <fstream>
#include <sstream>

#include <fmt/core.h>
#include <fmt/chrono.h>
#include <future>
#include "rat/system/types.hpp"
#include "rat/ThreadPool.hpp"

namespace rat::system {

    // -------------------------------
    // Time & Date
    // -------------------------------
    std::string getCurrentDateTime(void);
    std::string getCurrentTime(void);
    std::string getCurrentDate(void);
    std::string getCurrentDateTimeWithMs(void);

    std::string getCurrentDateTime_Underscored(void);
    // -------------------------------
    // System Info
    // -------------------------------
    std::string getUserName(void);
    std::string getOsInfo(void);
    std::string getConnectedPeripherals(void); // May require elevated privileges
    std::string getDisks(void);                 // May require elevated privileges
    std::string getCpuInfo(void);
    std::string getMemoryInfo(void);
    std::string getNetworkInfo(void);           // May require elevated privileges
    std::string getEnvVar(const std::string& arg_Name);
    bool setEnvVar(const std::string& arg_Name, const std::string& arg_Value); // May require elevated privileges

    // Check if current process is running with elevated privileges (root/admin)
    bool isElevatedPrivileges(void);

    // -------------------------------
    // Filesystem Queries
    // -------------------------------

    std::filesystem::path normalizePath(const std::string& Unix_Style_Path);

    bool exists(const std::filesystem::path& arg_Path);
    std::string pwd(void);
    std::string ls(const std::filesystem::path& arg_Path);
    std::string tree(const std::filesystem::path& Dir_Path);
    bool isFile(const std::filesystem::path& arg_Path);
    bool isDir(const std::filesystem::path& arg_Path);

    // -------------------------------
    // Filesystem Navigation
    // -------------------------------
    std::string cd(const std::filesystem::path& arg_Path);

    // -------------------------------
    // File Operations
    // -------------------------------
    bool createFile(const std::filesystem::path& File_Path); // May require elevated privileges
    bool copyFile(const std::filesystem::path& Origin_Path, const std::filesystem::path& Target_Path); // May require elevated privileges
    bool renameFile(const std::filesystem::path& Original_Path, const std::filesystem::path& Target_Path); // May require elevated privileges
    bool removeFile(const std::filesystem::path& File_Path); // May require elevated privileges
    size_t getFileSize(const std::filesystem::path& File_Path);
    std::string readFile(const std::filesystem::path& File_Path); // May require elevated privileges
    bool echo(const std::string& arg_Buffer, const std::filesystem::path& File_Path); // May require elevated privileges
    bool echo(const std::vector<std::string>& arg_Buffers, const std::filesystem::path& File_Path);
    
    bool appendToFile(const std::string& arg_Buffer, const std::filesystem::path& File_Path); // May require elevated privileges


    bool readBytesFromFile(const std::filesystem::path& File_Path, std::vector<uint8_t>& arg_Buffer);

    bool writeBytesIntoFile(const std::filesystem::path& File_Path, const std::vector<uint8_t>& arg_Buffer);
    // -------------------------------
    // Directory Operations
    // -------------------------------
    bool removeDir(const std::filesystem::path& Dir_Path); // May require elevated privileges
                                                           //
    bool copyDir(const std::filesystem::path& Original_Dir, const std::filesystem::path& Target_Dir);
    bool moveDir(const std::filesystem::path& Original_Path, const std::filesystem::path& Target_Path); // May require elevated privileges
    size_t getDirSize(const std::filesystem::path& Dir_Path);
    bool createDir(const std::filesystem::path& Dir_Path); // May require elevated privileges

    // -------------------------------
    // Process Management
    // -------------------------------
    /*boost_wrapper*/

    uint32_t getProcessId(void);
    std::string listProcesses(void); // May require elevated privileges
    bool killProcess(uint32_t arg_Pid); // May require elevated privileges
    
    std::string runShellCommand(const std::string& arg_Command, unsigned int Timeout_ms);
    
    std::future<std::string> runShellCommand(const std::string& arg_Command, unsigned int Timeout_ms, rat::ThreadPool& Timer_Pool);    
    
    struct ProcessResult {
        int exit_code;
        std::string stdout_str;
        std::string stderr_str;
    };
    
    std::future<rat::system::ProcessResult> runProcessAsync(const std::string& arg_Command, unsigned int Timeout_ms);
    std::future<rat::system::ProcessResult> runProcessAsync(const std::string& arg_Command, unsigned int Timeout_ms, rat::ThreadPool& Timer_Pool);
   

    /*Low level Operations*/

} // namespace rat::system
