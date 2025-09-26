#pragma once

#include <fmt/core.h>

// ===========================
// Handler Logging
// ===========================
#ifdef DEBUG_RAT_HANDLER
#define HANDLER_DEBUG_LOG(...)                                                                     \
    do {                                                                                           \
        fmt::print("[HANDLER:DEBUG] {}:{}: ", __FILE__, __LINE__);                                 \
        fmt::print(__VA_ARGS__);                                                                   \
        fmt::print("\n");                                                                          \
    } while(0)

#define HANDLER_ERROR_LOG(...)                                                                     \
    do {                                                                                           \
        fmt::print(stderr, "[HANDLER:ERROR] {}:{}: ", __FILE__, __LINE__);                         \
        fmt::print(stderr, __VA_ARGS__);                                                           \
        fmt::print(stderr, "\n");                                                                  \
    } while(0)
#else
#define HANDLER_DEBUG_LOG(...)                                                                     \
    do {                                                                                           \
    } while(0)
#define HANDLER_ERROR_LOG(...)                                                                     \
    do {                                                                                           \
    } while(0)
#endif
