#pragma once

#include <fmt/core.h>

// ===========================
// System Logging
// ===========================
#ifdef DEBUG_RAT_SYSTEM
#define SYSTEM_DEBUG_LOG(...)                                                                      \
    do {                                                                                           \
        fmt::print("[SYSTEM:DEBUG] {}:{}: ", __FILE__, __LINE__);                                  \
        fmt::print(__VA_ARGS__);                                                                   \
        fmt::print("\n");                                                                          \
    } while(0)

#define SYSTEM_ERROR_LOG(...)                                                                      \
    do {                                                                                           \
        fmt::print(stderr, "[SYSTEM:ERROR] {}:{}: ", __FILE__, __LINE__);                          \
        fmt::print(stderr, __VA_ARGS__);                                                           \
        fmt::print(stderr, "\n");                                                                  \
    } while(0)
#else
#define SYSTEM_DEBUG_LOG(...)                                                                      \
    do {                                                                                           \
    } while(0)
#define SYSTEM_ERROR_LOG(...)                                                                      \
    do {                                                                                           \
    } while(0)
#endif
