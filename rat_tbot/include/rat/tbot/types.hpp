#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <cstddef>
#include <vector>
#include <deque>
#include <optional>
#include "logging.hpp"

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
    std::optional<std::string> name;  // Present only for documents

    File() = default;
    File(const std::string& arg_Id, const std::string& Mime_Type, size_t arg_Size, std::optional<std::string> arg_Name = std::nullopt)
        : id(arg_Id), mime_type(Mime_Type), size(arg_Size), name(arg_Name) {}
};

struct Message {
    int64_t id;        // Message ID
    int64_t origin;    // User ID or Chat ID where message originated
    std::string text;
    std::vector<File> files;

    Message() = default;
    Message(int64_t arg_Id, int64_t arg_Origin, const std::string& arg_Text)
        : id(arg_Id), origin(arg_Origin), text(arg_Text) {}
};

struct Update {
    int64_t id;  // Update ID
    Message message;

    Update() = default;
    Update(int64_t arg_Id, const Message& msg)
        : id(arg_Id), message(msg) {}
};

struct UpdateDeque {
    std::deque<Update> dq;
    size_t max_size;
    
    UpdateDeque(size_t Max_Size) : max_size(Max_Size) {};
    
    void push(const Update& arg_Update) {
        if(this->dq.size() == this->max_size) this->dq.pop_front();
        dq.push_back(arg_Update);
    }

    Update pop(void) {
        if(this->dq.empty())   ERROR_LOG("popping an empty deque of updates ?");
        Update update = this->dq.back();
        this->dq.pop_back();
        return update; 
    }
    bool empty(void) const {return this->dq.empty();}
    size_t size(void) const {return this->dq.size();}

};
}  // namespace rat::tbot
