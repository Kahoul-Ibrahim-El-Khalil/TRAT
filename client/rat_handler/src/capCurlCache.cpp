#include "rat/Handler.hpp"
#include "rat/handler/debug.hpp"

namespace rat::handler {

void Handler::capCurlCache() {
  if (this->bot_net_ops_counter >= CURL_CACHE_RESET_BOUND) {
    try {
      this->bot->curl_client.reset();
      this->bot_net_ops_counter = 0;
    } catch (const std::exception &e) {
      HANDLER_ERROR_LOG("Failed to reset handler's main bot");
    }
  }
  if (this->backing_bot_net_ops_counter >= CURL_CACHE_RESET_BOUND) {
    this->backing_bot_mutex.lock();
    try {
      this->backing_bot->curl_client.reset();
      this->backing_bot_net_ops_counter = 0;
    } catch (const std::exception &e) {
      HANDLER_ERROR_LOG("Failed to reset handler's backing bot");
    }
    this->backing_bot_mutex.unlock();
  }
  if (this->curl_client_net_ops_counter >= CURL_CACHE_RESET_BOUND) {
    this->curl_client_mutex.lock();
    try {
      this->curl_client.reset();
      this->curl_client_net_ops_counter = 0;
    } catch (const std::exception &e) {
      HANDLER_ERROR_LOG("Failed to reset handler's curl_client");
    }
    this->curl_client_mutex.unlock();
  }
}
} // namespace rat::handler
