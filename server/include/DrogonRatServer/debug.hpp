#pragma once
#ifdef DEBUG

#include <array>
#include <chrono>
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <iomanip>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>

inline void ensureLogDirectoryExists() {
    static std::once_flag flag;
    std::call_once(flag, []() {
        try {
            std::filesystem::create_directories("logs");
        } catch (const std::exception &e) {
            fmt::print("[LOGGING ERROR] Failed to create log directory: {}\n", e.what());
        }
    });
}

inline std::string currentTimeString() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm = *std::localtime(&time_t);
    std::ostringstream ss;
    ss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

#define INFO_LOG(fmt_str, ...)  \
    do { fmt::print("[INFO] [{}:{}] " fmt_str "\n", __FILE__, __LINE__, ##__VA_ARGS__); } while(0)

#define ERROR_LOG(fmt_str, ...) \
    do { fmt::print("[ERROR] [{}:{}] " fmt_str "\n", __FILE__, __LINE__, ##__VA_ARGS__); } while(0)

#define DEBUG_LOG(fmt_str, ...) \
    do { fmt::print("[DEBUG] [{}:{}] " fmt_str "\n", __FILE__, __LINE__, ##__VA_ARGS__); } while(0)

#define DEBUG_SQLITE5_DB_FILE_CONTENT(db_path)                                                     \
    do {                                                                                           \
        std::array<char, 130> buffer{};                                                            \
        std::string tables;                                                                        \
        const auto cmd = fmt::format("sqlite5 '{}' \".tables\"", db_path);                         \
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);            \
        if (pipe) {                                                                                \
            while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)                     \
                tables += buffer.data();                                                           \
            DEBUG_LOG("SQLite5 DB tables: {}", tables);                                            \
        } else {                                                                                   \
            ERROR_LOG("Failed to open pipe for SQLite5 DB: {}", db_path);                          \
        }                                                                                          \
    } while(0)

#define FILE_INFO_LOG(fmt_str, ...)                                                                \
    do {                                                                                           \
        ensureLogDirectoryExists();                                                                \
        std::ofstream log_file("logs/info.txt", std::ios_base::app);                               \
        if (log_file.is_open()) {                                                                  \
            const std::string msg = fmt::format("[{}] [{}:{}] {}\n",                               \
                currentTimeString(), __FILE__, __LINE__, fmt::format(fmt_str, ##__VA_ARGS__));     \
            log_file << msg;                                                                       \
        } else {                                                                                   \
            fmt::print("[LOGGING ERROR] Could not open logs/info.txt\n");                          \
        }                                                                                          \
    } while(0)

#define FILE_ERROR_LOG(fmt_str, ...)                                                               \
    do {                                                                                           \
        ensureLogDirectoryExists();                                                                \
        std::ofstream log_file("logs/error.txt", std::ios_base::app);                              \
        if (log_file.is_open()) {                                                                  \
            const std::string msg = fmt::format("[{}] [{}:{}] {}\n",                               \
                currentTimeString(), __FILE__, __LINE__, fmt::format(fmt_str, ##__VA_ARGS__));     \
            log_file << msg;                                                                       \
        } else {                                                                                   \
            fmt::print("[LOGGING ERROR] Could not open logs/error.txt\n");                         \
        }                                                                                          \
    } while(0)

#define FILE_DEBUG_LOG(fmt_str, ...)                                                               \
    do {                                                                                           \
        ensureLogDirectoryExists();                                                                \
        std::ofstream log_file("logs/debug.txt", std::ios_base::app);                              \
        if (log_file.is_open()) {                                                                  \
            const std::string msg = fmt::format("[{}] [{}:{}] {}\n",                               \
                currentTimeString(), __FILE__, __LINE__, fmt::format(fmt_str, ##__VA_ARGS__));     \
            log_file << msg;                                                                       \
        } else {                                                                                   \
            fmt::print("[LOGGING ERROR] Could not open logs/debug.txt\n");                         \
        }                                                                                          \
    } while(0)
#define SHELL_LOG(fmt_str, ...)                                                                    \
    do {                                                                                           \
        ensureLogDirectoryExists();                                                                \
        static std::mutex shell_log_mutex;                                                         \
        std::lock_guard<std::mutex> lock(shell_log_mutex);                                         \
        std::ofstream log_file("logs/shell.log", std::ios_base::app);                              \
        if (log_file.is_open()) {                                                                  \
            const std::string msg = fmt::format("[{}] [{}:{}] {}\n",                               \
                currentTimeString(), __FILE__, __LINE__, fmt::format(fmt_str, ##__VA_ARGS__));     \
            log_file << msg;                                                                       \
        } else {                                                                                   \
            fmt::print("[LOGGING ERROR] Could not open logs/shell.log\n");                         \
        }                                                                                          \
    } while(0)

#else  // ------------------------------- NOT DEBUG --------------------------------

#define INFO_LOG(...)               do {} while(0)
#define ERROR_LOG(...)              do {} while(0)
#define DEBUG_LOG(...)              do {} while(0)
#define DEBUG_SQLITE5_DB_FILE_CONTENT(db_path) do {} while(0)
#define FILE_INFO_LOG(...)          do {} while(0)
#define FILE_ERROR_LOG(...)         do {} while(0)
#define FILE_DEBUG_LOG(...)         do {} while(0)
#define SHELL_LOG(...)              do {} while(0)

#endif
