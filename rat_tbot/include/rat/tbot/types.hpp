/*rat_tbot/include/tbot/tbot.hpp*/
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <cstddef>
#include <vector>
#include <deque>
#include <optional>
#include <simdjson.h>

namespace rat::tbot {

  enum class BotResponse {
    SUCCESS,
    NO_FILE,
    CONNECTION_ERROR,
    NETWORK_ERROR,
    PARSE_ERROR,
    UNAUTHORIZED,
    UNKNOWN_ERROR
};

struct File {
    std::string id;
    std::string mime_type;
    size_t size;
    std::optional<std::string> name;

    File() = default;
    File(const std::string& arg_Id, const std::string& Mime_Type, size_t arg_Size,
         std::optional<std::string> arg_Name = std::nullopt)
        : id(arg_Id), mime_type(Mime_Type), size(arg_Size), name(arg_Name) {}

    File(const File&) = default;
    File(File&&) noexcept = default;
};

struct Message {
    int64_t id;
    int64_t origin;
    std::string text;
    std::vector<File> files;
    std::optional<std::string> caption;

    Message() = default;
    Message(int64_t arg_Id, int64_t arg_Origin, const std::string& arg_Text)
        : id(arg_Id), origin(arg_Origin), text(arg_Text) {}

    // Copy
    Message(const Message& other) = default;
    Message& operator=(const Message& other) = default;

    // Move
    Message(Message&& other) noexcept = default;
    Message& operator=(Message&& other) noexcept = default;
};
struct Update {
    int64_t id;
    Message message;

    Update() = default;
    Update(int64_t arg_Id, const Message& msg) : id(arg_Id), message(msg) {}

    Update(const Update& other) = default;
    Update& operator=(const Update& other) = default;

    Update(Update&& other) noexcept = default;
    Update& operator=(Update&& other) noexcept = default;
};

}  // namespace rat::tbot
