#pragma once

#ifdef DEBUG_RAT_NETWORKING
#include <fmt/core.h>

#define NETWORKING_DEBUG_LOG(...)                                                                  \
    do {                                                                                           \
        fmt::print("[DEBUG] {}:{}: ", __FILE__, __LINE__);                                         \
        fmt::print(__VA_ARGS__);                                                                   \
        fmt::print("\n");                                                                          \
    } while(0)

#define NETWORKING_ERROR_LOG(...)                                                                  \
    do {                                                                                           \
        fmt::print("[ERROR] {}:{}: ", __FILE__, __LINE__);                                         \
        fmt::print(__VA_ARGS__);                                                                   \
        fmt::print("\n");                                                                          \
    } while(0)

#else

#define NETWORKING_DEBUG_LOG(...)                                                                  \
    do {                                                                                           \
    } while(0)
#define NETWORKING_ERROR_LOG(...)                                                                  \
    do {                                                                                           \
    } while(0)

#endif
