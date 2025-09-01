#pragma once

#ifdef DEBUG_RAT_NETWORKING
    #include <spdlog/spdlog.h>
    #include <spdlog/sinks/stdout_color_sinks.h>

    namespace {
        struct _rat_logging_initializer {
            _rat_logging_initializer() {
                spdlog::set_level(spdlog::level::debug);
                spdlog::set_pattern("[%H:%M:%S] [%^%l%$] %v");
            }
        } _rat_logging_initializer_instance;
    }

    #define DEBUG_LOG(...) spdlog::debug(__VA_ARGS__)
    #define ERROR_LOG(...) spdlog::error(__VA_ARGS__)
#else
    #define DEBUG_LOG(...) ((void)0)
    #define ERROR_LOG(...) ((void)0)
#endif


