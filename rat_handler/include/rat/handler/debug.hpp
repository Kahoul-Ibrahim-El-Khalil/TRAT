#pragma once

#ifdef DEBUG_RAT_HANDLER
    #include <spdlog/spdlog.h>
    #include <spdlog/sinks/stdout_color_sinks.h>
    // Ensure spdlog is initialized exactly once, header-only
    inline void _rat_init_logging() {
        static bool initialized = false;
        if (!initialized) {
            spdlog::set_level(spdlog::level::debug);           
            // Add module name "rat_handler" in the prefix
            spdlog::set_pattern("[%H:%M:%S] [%^%l%$] [rat_handler] %v"); 
            initialized = true;
        }
    }

    #define DEBUG_LOG(...) do { _rat_init_logging(); spdlog::debug(__VA_ARGS__); } while(0)
    #define ERROR_LOG(...) do { _rat_init_logging(); spdlog::error(__VA_ARGS__); } while(0)

#else
    #define DEBUG_LOG(...) do { } while(0)
    #define ERROR_LOG(...) do { } while(0)
#endif
