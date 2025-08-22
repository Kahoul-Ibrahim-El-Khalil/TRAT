#pragma once
#include "rat/tbot/macros.hpp"
#include <array>

namespace rat::tbot {
using MessageResponseBuffer      = std::array<char, TELEGRAM_BOT_API_MESSAGE_RESPONSE_BUFFER_SIZE>;
using FileOperationResponseBuffer = std::array<char, TELEGRAM_BOT_API_FILE_OPREATION_RESPONSE_BUFFER_SIZE>;
using UpdateBuffer               = std::array<char, TELEGRAM_BOT_API_UPDATE_BUFFER_SIZE>;

using json = nlohmann::json;
}// namespace rat::tbot;
