#pragma once

#ifdef DEBUG_RAT_NETWORKING
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

static void _rat_networking_init_logging() {
	static bool is_rat_networking_logging_initialized = false;
		if(!is_rat_networking_logging_initialized) {
			spdlog::set_level(spdlog::level::debug);
			// Add module name "rat_handler" in the prefix
			spdlog::set_pattern("[%H:%M:%S] [%^%l%$] [rat_networking] %v");
			is_rat_networking_logging_initialized = true;
		}
}

#define NETWORKING_DEBUG_LOG(...)                                              \
		do {                                                                   \
			_rat_networking_init_logging();                                    \
			spdlog::debug(__VA_ARGS__);                                        \
	} while(0)
#define NETWORKING_ERROR_LOG(...)                                              \
		do {                                                                   \
			_rat_networking_init_logging();                                    \
			spdlog::error(__VA_ARGS__);                                        \
	} while(0)

#else
#define NETWORKING_DEBUG_LOG(...)                                              \
		do {                                                                   \
	} while(0)
#define NETWORKING_ERROR_LOG(...)                                              \
		do {                                                                   \
	} while(0)
#endif
