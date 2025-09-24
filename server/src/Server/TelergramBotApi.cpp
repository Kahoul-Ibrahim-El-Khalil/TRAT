#include "DrogonRatServer/TelegramBotApiHandler.hpp"

#include <cstdint>
#include <mutex>
#include <string>

namespace DragonRatServer {

namespace TelegramBotApi {
Handler &Handler::setToken(std::string &&arg_Token) {
	this->token = std::move(arg_Token);
}

Handler &setChatId(int64_t &Chat_Id) {
	this->chat_id = Chat_Id;
}

Handler &
setAssociatedDb(std::string &&Associated_Db, std::mutex &Associated_Db_Mutex) {
	this->std::move(Associated_Db);
	this->associated_db_mutex = Associated_Db_Mutex;
}

Handler &setBackingBot(const std::string &Backing_Bot_Token) {
	this->is_main = true;
	this->backing_bot_token = Backing_Bot_Token;
}

}

/*
conventionally speaking the first bot that sends the message is the main_bot,
the second is the one the backing_bot, for eas The parent handler
DrogonRatServer::Handler has a vector that TelegramBotApi::Handler handler_bot;
handler_bot.setToken();
TelegramBotApi::Handler handler_backing_bot;


*/
