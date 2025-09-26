
#pragma once
#ifdef DEBUG_RAT_TBOT
#include <fmt/core.h>

#define TBOT_DEBUG_LOG(...)                                                                        \
    do {                                                                                           \
        fmt::print("[DEBUG] {}:{}: ", __FILE__, __LINE__);                                         \
        fmt::print(__VA_ARGS__);                                                                   \
        fmt::print("\n");                                                                          \
    } while(0)

#define TBOT_ERROR_LOG(...)                                                                        \
    do {                                                                                           \
        fmt::print("[ERROR] {}:{}: ", __FILE__, __LINE__);                                         \
        fmt::print(__VA_ARGS__);                                                                   \
        fmt::print("\n");                                                                          \
    } while(0)

#else

#define TBOT_DEBUG_LOG(...)                                                                        \
    do {                                                                                           \
    } while(0)
#define TBOT_ERROR_LOG(...)                                                                        \
    do {                                                                                           \
    } while(0)

#endif
