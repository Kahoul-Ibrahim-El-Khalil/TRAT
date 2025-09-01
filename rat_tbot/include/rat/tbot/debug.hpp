#pragma once

#ifdef DEBUG_RAT_TBOT
    #include <spdlog/spdlog.h>
    #include <spdlog/sinks/stdout_color_sinks.h>
    struct _rat_logging_initializer {
        _rat_logging_initializer() {
            spdlog::set_level(spdlog::level::debug); // Show debug and above
            spdlog::set_pattern("[%H:%M:%S] [%^%l%$] %v"); // Optional: pretty format
        }
    } _rat_logging_initializer_instance;

    #define DEBUG_LOG(...) spdlog::debug(__VA_ARGS__)
    #define ERROR_LOG(...) spdlog::error(__VA_ARGS__)
#else
    #define DEBUG_LOG(...) do { } while(0)
    #define ERROR_LOG(...) do { } while(0)
#endif
