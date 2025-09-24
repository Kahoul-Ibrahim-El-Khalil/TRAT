#pragma once

#include <fmt/core.h>

#ifdef DEBUG

#define INFO_LOG(fmt_str, ...)                                                 \
		do {                                                                   \
			fmt::print("[INFO] [{}:{}] " fmt_str "\n", __FILE__, __LINE__,     \
			           ##__VA_ARGS__);                                         \
	} while(0)

#define ERROR_LOG(fmt_str, ...)                                                \
		do {                                                                   \
			fmt::print("[ERROR] [{}:{}] " fmt_str "\n", __FILE__, __LINE__,    \
			           ##__VA_ARGS__);                                         \
	} while(0)

#define DEBUG_LOG(fmt_str, ...)                                                \
		do {                                                                   \
			fmt::print("[DEBUG] [{}:{}] " fmt_str "\n", __FILE__, __LINE__,    \
			           ##__VA_ARGS__);                                         \
	} while(0)

#else

#define INFO_LOG(fmt_str, ...)                                                 \
		do {                                                                   \
	} while(0)
#define DEBUG_LOG(fmt_str, ...)                                                \
		do {                                                                   \
	} while(0)
#define ERROR_LOG(fmt_str, ...)                                                \
		do {                                                                   \
	} while(0)

#endif
