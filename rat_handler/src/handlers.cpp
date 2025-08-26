/*rat_handler/src/handlers.cpp*/
#include "rat/Handler.hpp"


#include <vector>
#include <sstream>
#include <fmt/core.h>
#include <fmt/chrono.h>
#include <filesystem>
#include <algorithm>
#include "logging.hpp"



namespace rat::handler {

void Handler::parseTelegramMessageToCommand() {
    std::string message_text = telegram_update.message.text;
    __normalizeWhiteSpaces(message_text);

    auto tokens = splitArguments(message_text);
    if (tokens.empty()) return ;

    std::string directive = stripQuotes(tokens[0]);

    std::vector<std::string> parameters;
    for (size_t i = 1; i < tokens.size(); ++i) {
        parameters.push_back(stripQuotes(tokens[i]));
    }

    this->command = Command(std::move(directive), std::move(parameters));
    return;
}



void Handler::handleSetCommand() {
    if (command.parameters.empty()) {
        this->bot.sendMessage("/set <variable> <value>");
        return;
    }

    const std::string& variable = command.parameters[0];
    if (variable == "update_interval") {
        if (command.parameters.size() < 2) {
            this->bot.sendMessage("Missing value for update_interval");
            return;
        }

        try {
            uint16_t update_interval = _stringToUint16(command.parameters[1]);
            this->bot.setUpdateIterval(update_interval);
            this->bot.sendMessage(fmt::format("Bot's update_interval == {}", update_interval));
        }
        catch (const std::exception& e) {
            this->bot.sendMessage(fmt::format("Error setting update_interval: {}", e.what()));
        }
    } else {
        this->bot.sendMessage(fmt::format("Unknown variable: {}", variable));
    }
}

} // namespace rat::handler
