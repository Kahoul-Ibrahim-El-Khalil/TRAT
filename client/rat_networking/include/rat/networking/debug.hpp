#pragma once

#ifdef DEBUG_RAT_NETWORKING
#include <fmt/core.h>
#include <iostream>
#define NETWORKING_DEBUG_LOG(...)                                              \
		do {                                                                   \
			std::cout << fmt::format("[DEBUG] {}:{}: ", __FILE__, __LINE__);   \
			std::cout << fmt::format(__VA_ARGS__) << std::endl;                \
	} while(0)

#define NETWORKING_ERROR_LOG(...)                                              \
		do {                                                                   \
			std::cout << fmt::format("[ERROR] {}:{}: ", __FILE__, __LINE__);   \
			std::cout << fmt::format(__VA_ARGS__) << std::endl;                \
	} while(0)

#else

#define NETWORKING_DEBUG_LOG(...)                                              \
		do {                                                                   \
	} while(0)
#define NETWORKING_ERROR_LOG(...)                                              \
		do {                                                                   \
	} while(0)

#endif
