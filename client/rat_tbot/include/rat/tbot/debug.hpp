#pragma once
#ifdef DEBUG
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#endif
#ifdef DEBUG_RAT_TBOT
static void _rat_tbot_init_logging() {
	static bool is_rat_tbot_logging_initialized = false;
		if(!is_rat_tbot_logging_initialized) {
			spdlog::set_level(spdlog::level::debug);
			// Add module name "rat_handler" in the prefix
			spdlog::set_pattern("[%H:%M:%S] [%^%l%$] [rat_tbot] %v");
			is_rat_tbot_logging_initialized = true;
		}
}

#define TBOT_DEBUG_LOG(...)                                                    \
		do {                                                                   \
			_rat_tbot_init_logging();                                          \
			spdlog::debug(__VA_ARGS__);                                        \
	} while(0)
#define TBOT_ERROR_LOG(...)                                                    \
		do {                                                                   \
			_rat_tbot_init_logging();                                          \
			spdlog::error(__VA_ARGS__);                                        \
	} while(0)

#else
#define TBOT_DEBUG_LOG(...)                                                    \
		do {                                                                   \
	} while(0)
#define TBOT_ERROR_LOG(...)                                                    \
		do {                                                                   \
	} while(0)
#endif
