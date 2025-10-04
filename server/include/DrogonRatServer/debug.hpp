#pragma once

#include <array>
#include <chrono>
#include <fmt/core.h>
#include <fstream>
#include <iomanip>
#include <memory>
#include <mutex>
#include <string>

#ifdef DEBUG

#define INFO_LOG(fmt_str, ...)                                                                     \
    do {                                                                                           \
        fmt::print("[INFO] [{}:{}] " fmt_str "\n", __FILE__, __LINE__, ##__VA_ARGS__);             \
    } while(0)

#define ERROR_LOG(fmt_str, ...)                                                                    \
    do {                                                                                           \
        fmt::print("[ERROR] [{}:{}] " fmt_str "\n", __FILE__, __LINE__, ##__VA_ARGS__);            \
    } while(0)

#define DEBUG_LOG(fmt_str, ...)                                                                    \
    do {                                                                                           \
        fmt::print("[DEBUG] [{}:{}] " fmt_str "\n", __FILE__, __LINE__, ##__VA_ARGS__);            \
    } while(0)

#define DEBUG_SQLITE3_DB_FILE_CONTENT(db_path)                                                     \
    do {                                                                                           \
        std::array<char, 128> buffer{};                                                            \
        std::string tables;                                                                        \
        auto cmd = fmt::format("sqlite3 '{}' \".tables\"", db_path);                               \
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);            \
        if(pipe) {                                                                                 \
            while(fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)                      \
                tables += buffer.data();                                                           \
            DEBUG_LOG("SQLite3 DB tables: {}", tables);                                            \
        } else {                                                                                   \
            ERROR_LOG("Failed to open pipe for SQLite3 DB: {}", db_path);                          \
        }                                                                                          \
    } while(0)

#define FILE_LOG(filename, fmt_str, ...)                                                           \
    do {                                                                                           \
        static std::mutex log_mutex;                                                               \
        std::lock_guard<std::mutex> lock(log_mutex);                                               \
        std::ofstream log_file(filename, std::ios_base::app);                                      \
        if(log_file.is_open()) {                                                                   \
            auto now = std::chrono::system_clock::now();                                           \
            auto time_t = std::chrono::system_clock::to_time_t(now);                               \
            log_file << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")         \
                     << "] ";                                                                      \
            log_file << fmt::format(fmt_str, ##__VA_ARGS__) << std::endl;                          \
        }                                                                                          \
    } while(0)

#define FILE_INFO_LOG(fmt_str, ...) FILE_LOG("info.txt", fmt_str, ##__VA_ARGS__)
#define FILE_ERROR_LOG(fmt_str, ...) FILE_LOG("error.txt", fmt_str, ##__VA_ARGS__)
#define FILE_DEBUG_LOG(fmt_str, ...) FILE_LOG("debug.txt", fmt_str, ##__VA_ARGS__)
#define SHELL_LOG(fmt_str, ...) FILE_LOG("shell.log", fmt_str, ##__VA_ARGS__)

#else

#define INFO_LOG(...)                                                                              \
    do {                                                                                           \
    } while(0)
#define ERROR_LOG(...)                                                                             \
    do {                                                                                           \
    } while(0)
#define DEBUG_LOG(...)                                                                             \
    do {                                                                                           \
    } while(0)
#define DEBUG_SQLITE3_DB_FILE_CONTENT(db_path)                                                     \
    do {                                                                                           \
    } while(0)
#define FILE_INFO_LOG(...)                                                                         \
    do {                                                                                           \
    } while(0)
#define FILE_ERROR_LOG(...)                                                                        \
    do {                                                                                           \
    } while(0)
#define FILE_DEBUG_LOG(...)                                                                        \
    do {                                                                                           \
    } while(0)
#define SHELL_LOG(...)                                                                             \
    do {                                                                                           \
    } while(0)

#endif
