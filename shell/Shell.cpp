/*Shell.hpp*/
#include "Shell.hpp"

#include <ctime>
#include <filesystem>
#include <iostream>
#include <json/writer.h>
#include <regex>

void Shell::interact() {
    INFO_LOG("Interactive Telegram-like shell started. Type /exit to quit.");
    INFO_LOG("Usage examples:");
    INFO_LOG("  hello bot");
    INFO_LOG("  --file=/path/to/file.txt --caption=My caption");

    std::string input;
    while(true) {
        std::cout << "You> ";
        if(!std::getline(std::cin, input))
            break;

        if(input.empty())
            continue;

        if(input == ".q") {
            INFO_LOG("Exiting shell.");
            break;
        }

        // Parse for --file and --caption arguments
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

        Json::StreamWriterBuilder wbuilder;
        wbuilder["indentation"] = ""; // compact JSON
        std::string json_str = Json::writeString(wbuilder, update);

#ifdef DEBUG
        DEBUG_LOG("Generated update:\n{}", json_str);
#else
        std::cout << json_str << "\n";
        std::cout.flush();
#endif
    }
}

Json::Value Shell::makeTextUpdate(std::string &&arg_Text) {
    ++this->update_offset;
    ++this->message_offset;

    Json::Value update;
    update["update_id"] = this->update_offset;

    Json::Value message;
    message["message_id"] = this->message_offset;
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

Json::Value Shell::makeFileUpdate(std::string &&File_Path, std::string &&arg_Caption) {
    ++this->update_offset;
    ++this->message_offset;

    Json::Value update;
    update["update_id"] = this->update_offset;

    Json::Value message;
    message["message_id"] = this->message_offset;

    if(!arg_Caption.empty())
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
