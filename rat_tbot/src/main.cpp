#include "logging.hpp"
#include "rat/tbot/tbot.hpp"

#include <fmt/core.h>
#include <fmt/chrono.h>

const std::string TOKEN = "5598026972:AAFtLSUld27ImilkcOppvGMFgSAPXozEgVo"; 
constexpr int64_t     MASTER_ID = 1492536442; 

static std::string init_message = "";

int main(void) {
    rat::tbot::Bot bot(TOKEN, MASTER_ID);
    fmt::print("Hello world!\n");
    bot.sendMessage("Hello");
    return 0;
}