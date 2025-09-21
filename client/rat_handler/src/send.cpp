#include "rat/Handler.hpp"
#include "rat/handler/debug.hpp"
#include "rat/tbot/types.hpp"

namespace rat::handler {

void Handler::sendMessage(const std::string &Message_Text) {
  if (this->bot->sendMessage(Message_Text) ==
      ::rat::tbot::BotResponse::SUCCESS) {
    HANDLER_DEBUG_LOG("Succeeded at sending: {}", Message_Text);
    this->bot_net_ops_counter++;
  } else {
    HANDLER_ERROR_LOG("Failed at sending: {}", Message_Text);
  }
}

void Handler::sendBackingMessage(const std::string &Message_Text) {
  this->backing_bot_mutex.lock();
  if (this->backing_bot->sendMessage(Message_Text) ==
      ::rat::tbot::BotResponse::SUCCESS) {
    HANDLER_DEBUG_LOG("Succeeded at sending: {}", Message_Text);
    this->backing_bot_net_ops_counter++;
  } else {
    HANDLER_ERROR_LOG("Failed at sending: {}", Message_Text);
  }
  this->backing_bot_mutex.unlock();
}

// any MIME upload operation reset the curl_client to ensure proper and clean
// cache.
void Handler::sendFile(const std::filesystem::path &File_Path,
                       const std::string &arg_Caption) {
  if (this->bot->sendFile(File_Path, arg_Caption) ==
      ::rat::tbot::BotResponse::SUCCESS) {
    this->bot->curl_client.reset();
    this->bot_net_ops_counter = 0;
    HANDLER_DEBUG_LOG("Succeeded at sending file {}", File_Path.string());
  } else {
    HANDLER_ERROR_LOG("Failed at sending file {}", File_Path.string());
  }
}
} // namespace rat::handler
