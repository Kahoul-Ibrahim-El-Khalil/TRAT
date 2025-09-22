#pragma once

#include <format>
#include <iostream>

#ifdef DEBUG
#define INFO_LOG(fmt, ...)                                                     \
  std::clog << std::format("[INFO] [{}:{}] " fmt "\n", __FILE__, __LINE__,     \
                           ##__VA_ARGS__)

#define ERROR_LOG(fmt, ...)                                                    \
  std::cerr << std::format("[ERROR] [{}:{}] " fmt "\n", __FILE__, __LINE__,    \
                           ##__VA_ARGS__)

#define DEBUG_LOG(fmt, ...)                                                    \
  std::clog << std::format("[DEBUG] [{}:{}] " fmt "\n", __FILE__, __LINE__,    \
                           ##__VA_ARGS__)
#else
#define INFO_LOG(fmt, ...)                                                     \
  do {                                                                         \
  } while (0)

#define DEBUG_LOG(fmt, ...)                                                    \
  do {                                                                         \
  } while (0)
#define ERROR_LOG(fmt, ...)                                                    \
  do {                                                                         \
  } while (0)

#endif
