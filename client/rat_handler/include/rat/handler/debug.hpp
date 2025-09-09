#pragma once
#ifdef DEBUG
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#endif // DEBUG#endif

#ifdef DEBUG_RAT_HANDLER
static void _rat_handler_init_logging() {
	static bool is_handler_logging_initialized = false;
	if (!is_handler_logging_initialized) {
		spdlog::set_level(spdlog::level::debug);
		// Add module name "rat_handler" in the prefix
		spdlog::set_pattern("[%H:%M:%S] [%^%l%$] [rat_handler] %v");
		is_handler_logging_initialized = true;
	}
}

#define HANDLER_DEBUG_LOG(...)                                                                                                                       \
	do {                                                                                                                                             \
		_rat_handler_init_logging();                                                                                                                 \
		spdlog::debug(__VA_ARGS__);                                                                                                                  \
	} while (0)
#define HANDLER_ERROR_LOG(...)                                                                                                                       \
	do {                                                                                                                                             \
		_rat_handler_init_logging();                                                                                                                 \
		spdlog::error(__VA_ARGS__);                                                                                                                  \
	} while (0)

#else
#define HANDLER_DEBUG_LOG(...)                                                                                                                       \
	do {                                                                                                                                             \
	} while (0)
#define HANDLER_ERROR_LOG(...)                                                                                                                       \
	do {                                                                                                                                             \
	} while (0)
#endif
