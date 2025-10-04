#include "DrogonRatServer/Handler.hpp"
#include "DrogonRatServer/debug.hpp"

#include <cstdint>
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/orm/Mapper.h>
#include <fmt/core.h>
#include <fstream>
#include <functional>

namespace DrogonRatServer {
using HttpResponseCallback = std::function<void(const drogon::HttpResponsePtr &)>;

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
                                         FILE_INFO_LOG("Receiving as message:\n{}", arg_Message);
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
        FILE_ERROR_LOG("queryUpdateOffset() failed: {}", e.base().what());
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
        FILE_ERROR_LOG("queryMessageOffset() failed: {}", e.base().what());
        return 0;
    }
}
int64_t Handler::queryFileOffset() {
    try {
        const auto &result =
            p_db_client->execSqlSync("SELECT MAX(id) AS max_id FROM file;");

        if(!result.empty() && !result[0]["max_id"].isNull())
            return result[0]["max_id"].as<int64_t>();
        else
            return 0;

    } catch(const drogon::orm::DrogonDbException &e) {
        FILE_ERROR_LOG("queryFileOffset() failed: {}", e.base().what());
        return 0;
    }
}
void Handler::registerAll() {
    this->registerEchoHandler();
    this->registerTelegramBotApiHandler();
    this->registerUploadHandler();
}

} // namespace DrogonRatServer
