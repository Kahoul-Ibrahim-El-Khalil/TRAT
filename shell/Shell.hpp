/*Shell.hpp*/
#pragma once

#include <cstdint>
#include <json/value.h>
#include <string>

/*
 * Shell.hpp
 * ----------
 * This class implements a lightweight, interactive command-line shell that simulates
 * a Telegram Bot chat prompt. It is designed to be launched as a separate process
 * from the main DrogonRatServer and serves as a local input source for updates.
 *
 * The Shell continuously reads user input from the standard input stream and emits
 * corresponding JSON-formatted Telegram-style "update" objects to the standard output.
 * These JSON updates strictly follow the Telegram Bot API's structural conventions,
 * including fields such as "update_id", "message", "chat", and "from".
 *
 * Two types of messages are currently supported:
 *  1. Plain text messages (e.g. "hello bot")
 *  2. File messages (using --file=/path/to/file [--caption=optional text])
 *
 * The main DrogonRatServer's Handler component is responsible for launching this shell
 * process (e.g. via TinyProcessLibrary) and asynchronously reading its stdout stream.
 * Each line of output represents one complete Telegram-like update in JSON form, which
 * the Handler parses and pushes into its internal update queue for further processing.
 *
 * This design effectively mimics the behavior of Telegram’s message delivery pipeline:
 * the Shell acts as a "local user" sending updates,
 * the Handler acts as Telegram’s "update dispatcher",
 * and the DrogonRatServer acts as the "bot server" consuming and responding to updates.
 *
 * Typical usage:
 *   > hello bot
 *   > --file=/tmp/example.txt --caption=My file
 *
 * The shell runs indefinitely until ".q" is entered, at which point it gracefully exits.
 *
 * NOTE:
 * - The Shell process is independent and runs in its own console window.
 * - The output is machine-readable JSON meant to be consumed by another process, not humans.
 * - It provides a minimal, extendable simulation of Telegram interaction without
 *   requiring actual Telegram network communication.
 */

class Shell {
  private:
    int64_t update_offset;  // For update_id sequencing
    int64_t message_offset; // For message_id sequencing
    int64_t chat_id;        // Represents the user ID

  public:
    explicit Shell(int64_t Update_Offset, int64_t Message_Offset = 0, int64_t Chat_Id = 1003)
        : update_offset(Update_Offset), message_offset(Message_Offset), chat_id(Chat_Id) {
    }

    Shell(const Shell &) = delete;
    Shell &operator=(const Shell &) = delete;
    Shell(Shell &&) noexcept = delete;
    Shell &operator=(Shell &&) noexcept = delete;
    ~Shell() = default;

    void interact();

  private:
    Json::Value makeTextUpdate(std::string &&arg_Text);
    Json::Value makeFileUpdate(std::string &&File_Path, std::string &&arg_Caption);
};

#ifdef DEBUG
#include <fmt/core.h>
#define INFO_LOG(fmt_str, ...)                                                                     \
    fmt::print("[INFO]  [{}:{}] " fmt_str "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define ERROR_LOG(fmt_str, ...)                                                                    \
    fmt::print("[ERROR] [{}:{}] " fmt_str "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define DEBUG_LOG(fmt_str, ...)                                                                    \
    fmt::print("[DEBUG] [{}:{}] " fmt_str "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define INFO_LOG(fmt_str, ...)                                                                     \
    do {                                                                                           \
    } while(0)
#define DEBUG_LOG(fmt_str, ...)                                                                    \
    do {                                                                                           \
    } while(0)
#define ERROR_LOG(fmt_str, ...)                                                                    \
    do {                                                                                           \
    } while(0)
#endif
