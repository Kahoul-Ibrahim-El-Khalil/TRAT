#include "rat/networking.hpp"
#include "rat/tbot/tbot.hpp"

#include <fmt/core.h>
#include <iostream>
#include <string>
#include <thread>

#define TELEGRAM_BOT_API_BASE_URL "https://api.telegram.org/bot"

const std::string TOKEN = "5598026972:AAFtLSUld27ImilkcOppvGMFgSAPXozEgVo";
constexpr int64_t MASTER_ID = 1492536442;

int main(void) {
	::rat::tbot::BaseBot bot(TOKEN, MASTER_ID);
		for(auto i = 0; i < 10; ++i) {
			bot.sendMessage(fmt::format("Hello{}", i));
			std::this_thread::sleep_for(std::chrono::seconds(4));
		}
	return 0;
}

#undef TBOT_DEBUG_LOG
#undef TBOT_ERROR_LOG
#ifdef DEBUG_RAT_TBOT
#undef DEBUG_RAT_TBOT
#endif
