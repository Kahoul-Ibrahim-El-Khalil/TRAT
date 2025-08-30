/*rat_tbot/src/Bot.cpp*/
#include "rat/tbot/tbot.hpp"
#include "rat/tbot/types.hpp"
#include "simdjson.h"
#include <chrono>
#include <fmt/core.h>
#include <thread>

namespace rat::tbot {

Bot::Bot(const std::string& arg_Token, int64_t Master_Id, uint8_t Telegram_Connection_Timeout)
    : BaseBot(arg_Token, Master_Id, Telegram_Connection_Timeout)
{
    this->getting_update_url = fmt::format("{}{}/getUpdates?timeout={}&limit=1", TELEGRAM_BOT_API_BASE_URL, arg_Token, Telegram_Connection_Timeout);
}

File Bot::parseFile(simdjson::ondemand::value& file_val, const std::string& type) {
    File file;
    
    try {
        simdjson::ondemand::object file_obj;
        if (file_val.get_object().get(file_obj) != simdjson::SUCCESS) {
            ERROR_LOG("Failed to get file object");
            return file;
        }
        
        // Get file_id (required)
        std::string_view file_id_view;
        if (file_obj["file_id"].get_string().get(file_id_view) == simdjson::SUCCESS) {
            file.id = std::string(file_id_view);
        }
        
        // Get file_size (optional)
        uint64_t file_size;
        if (file_obj["file_size"].get_uint64().get(file_size) == simdjson::SUCCESS) {
            file.size = file_size;
        }
        
        // Get MIME type if available
        std::string_view mime_view;
        if (file_obj["mime_type"].get_string().get(mime_view) == simdjson::SUCCESS) {
            file.mime_type = std::string(mime_view);
        } else {
            // Set default MIME type based on file type
            if (type == "photo") file.mime_type = "image/jpeg";
            else if (type == "audio") file.mime_type = "audio/mpeg";
            else if (type == "video") file.mime_type = "video/mp4";
            else if (type == "voice") file.mime_type = "audio/ogg";
            else file.mime_type = "application/octet-stream";
        }
        
        // Get file name if available (usually for documents)
        std::string_view name_view;
        if (file_obj["file_name"].get_string().get(name_view) == simdjson::SUCCESS) {
            file.name = std::string(name_view);
        }
        
    } catch (const simdjson::simdjson_error& e) {
        ERROR_LOG("File parsing failed: {}", e.what());
    }
    
    return file;
}

void Bot::extractFilesFromMessage(simdjson::ondemand::object& message_obj, Message& message) {
    // Check for different file types that Telegram might send
    
    // Document
    simdjson::ondemand::value document_val;
    if (message_obj["document"].get(document_val) == simdjson::SUCCESS) {
        message.files.push_back(parseFile(document_val, "document"));
    }
    
    // Photo (array of photo sizes)
    simdjson::ondemand::value photo_val;
    if (message_obj["photo"].get(photo_val) == simdjson::SUCCESS) {
        simdjson::ondemand::array photo_array = photo_val.get_array();
        
        simdjson::ondemand::value actual_value;
        for (auto photo_size : photo_array) {
            if (photo_size.get(actual_value) == simdjson::SUCCESS) {
                message.files.push_back(parseFile(actual_value, "photo"));
            }   
        }
    }
    
    // Audio
    simdjson::ondemand::value audio_val;
    if (message_obj["audio"].get(audio_val) == simdjson::SUCCESS) {
        message.files.push_back(parseFile(audio_val, "audio"));
    }
    
    // Video
    simdjson::ondemand::value video_val;
    if (message_obj["video"].get(video_val) == simdjson::SUCCESS) {
        message.files.push_back(parseFile(video_val, "video"));
    }
    
    // Voice
    simdjson::ondemand::value voice_val;
    if (message_obj["voice"].get(voice_val) == simdjson::SUCCESS) {
        message.files.push_back(parseFile(voice_val, "voice"));
    }
}
// Improved parseMessage with better error handling
Message Bot::parseMessage(simdjson::ondemand::value& message_val) {
    Message message{};
    
    try {
        simdjson::ondemand::object message_obj;
        if (message_val.get_object().get(message_obj) != simdjson::SUCCESS) {
            ERROR_LOG("Failed to get message object");
            return message;
        }
        
        // Extract message_id (required field)
        if (message_obj["message_id"].get(message.id) != simdjson::SUCCESS) {
            ERROR_LOG("Missing message_id");
            return message;
        }
        
        // Extract from.id (required field)
        simdjson::ondemand::object from_obj;
        if (message_obj["from"].get_object().get(from_obj) == simdjson::SUCCESS) {
            from_obj["id"].get(message.origin);
        }
        
        // Extract text (optional)
        std::string_view text_view;
        if (message_obj["text"].get_string().get(text_view) == simdjson::SUCCESS) {
            message.text = std::string(text_view);
        }
        
        // Extract caption (optional)
        std::string_view caption_view;
        if (message_obj["caption"].get_string().get(caption_view) == simdjson::SUCCESS) {
            message.caption = std::string(caption_view);
        }
        
        // Extract files from different locations
        extractFilesFromMessage(message_obj, message);
        
    } catch (const simdjson::simdjson_error& e) {
        ERROR_LOG("Message parsing failed: {}", e.what());
    }
    
    return message;
}
Update Bot::parseJsonToUpdate() {
    Update update{};
    
    try {
        // Parse the JSON from our buffer
        simdjson::ondemand::document doc = simdjson_parser.iterate(this->http_buffer, this->recieved_data_size + simdjson::SIMDJSON_PADDING);
        
        // Check if response is OK
        bool ok;
        auto ok_error = doc["ok"].get(ok);
        if (ok_error != simdjson::SUCCESS || !ok) {
            ERROR_LOG("API response not OK");
            return {};
        }
        
        // Get the result array
        simdjson::ondemand::value result_val;
        if (doc["result"].get(result_val) != simdjson::SUCCESS) {
            DEBUG_LOG("No result field in response");
            return {};
        }
        
        simdjson::ondemand::array results;
        if (result_val.get_array().get(results) != simdjson::SUCCESS) {
            DEBUG_LOG("Result is not an array");
            return {};
        }
        
        // Process first update in array
        for (auto result_item : results) {
            simdjson::ondemand::object update_obj;
            if (result_item.get_object().get(update_obj) != simdjson::SUCCESS) continue;
            
            // Extract update_id
            int64_t update_id;
            if (update_obj["update_id"].get(update_id) != simdjson::SUCCESS) continue;
            
            update.id = update_id;
            this->setLastUpdateId(update.id);
            
            // Extract message
            simdjson::ondemand::value message_val;
            if (update_obj["message"].get(message_val) == simdjson::SUCCESS) {
                update.message = this->parseMessage(message_val);
            }
            
            break; // Only process first update since limit=1
        }
        
    } catch (const simdjson::simdjson_error& e) {
        ERROR_LOG("JSON parsing failed: {}", e.what());
    }
    
    return update;
}
Update Bot::getUpdate() {
    const std::string url = fmt::format("{}&offset={}&limit=1", getting_update_url, last_update_id + 1);

    DEBUG_LOG("Polling updates with offset: {}", this->last_update_id + 1);

    this->recieved_data_size = curl_client.sendHttpRequest(url, this->http_buffer, HTTP_RESPONSE_BUFFER_SIZE + simdjson::SIMDJSON_PADDING);

    if (this->recieved_data_size == 0) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return {};
    }
    /*Set the padding bytes to 0*/
    std::memset(this->http_buffer + this->recieved_data_size, 0, simdjson::SIMDJSON_PADDING);

    return this->parseJsonToUpdate();
}
}//namespace rat::tbot
