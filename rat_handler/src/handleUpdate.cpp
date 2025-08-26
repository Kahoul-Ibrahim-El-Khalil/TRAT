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
    if(t_message.text[0] != '!' && t_message.text[0] != '/') {
        this->bot.sendMessage("Empty Message, an integrated command starts with /, a dynamic one with ! ");
        return;
    }
 // Example: dynamic command prefix, say messages starting with "!"
    else if (t_message.text[0] == '!') {
        this->dispatchDynamicCommand();
    }

    else if (t_message.text[0] == '/') {
        this->dispatchIntegratedCommand();
    }
}

void Handler::dispatchDynamicCommand() {
    std::string& command_text = this->telegram_update.message.text;

    if (!command_text.starts_with("!")) {
        return; 
    }

    size_t first_space = command_text.find(' ');
    if (first_space == std::string::npos) {
        this->bot.sendMessage("Usage: !<command> <timeout> <args...>");
        return;
    }

    std::string directive = command_text.substr(1, first_space - 1); // remove '!'
    std::string rest = command_text.substr(first_space + 1);
    boost::trim(rest);

    // Lookup in map
    auto it = this->state.command_path_map.find(directive);
    if (it == this->state.command_path_map.end()) {
        this->bot.sendMessage(fmt::format("Unknown command: '{}'", directive));
        return;
    }

    std::filesystem::path& real_path = it->second;

    // Rewrite update message into /process form
    // "/process <timeout> <real_path> <args>"
    std::string new_message = fmt::format("{}{}", "/process " , rest);
    new_message.insert(new_message.find(' ', 9) + 1, real_path.string() + " "); 

    this->telegram_update.message.text = new_message;

    // Delegate to existing handler
    this->parseAndHandleProcessCommand();
}
// Dispatch method
void Handler::dispatchIntegratedCommand() {
    // Parse once and store in class member for handlers that need it
    DEBUG_LOG("Dispatching commands based on the update {}", this->telegram_update.id);
    this->parseTelegramMessageToCommand();
    
    for (const auto& handler : command_map) {
        if (this->command.directive == handler.command) {
            (this->*handler.handler)();
            return;
        }
    }
    this->bot.sendMessage(fmt::format("Unknown command: {}", command.directive));
}

} // namespace rat::handler
