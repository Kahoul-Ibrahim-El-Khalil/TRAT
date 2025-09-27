/*include/DrogonRatServer/Handler.hpp*/
#pragma once
#include "DrogonRatServer/debug.hpp"

#include <array>
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>
#include <drogon/orm/Mapper.h>
#include <filesystem>
#include <functional>
#include <string_view>

namespace DrogonRatServer {
struct Bot {
    int64_t id;
    std::string token;
};

// Forward declaration
class Handler;

using DrogonHandlerCallback = std::function<void(const drogon::HttpResponsePtr &)>;
using HttpResponseCallback = std::function<void(const drogon::HttpResponsePtr &)>;

namespace TelegramBotApi {
constexpr char SUCCESS_JSON_RESPONSE[] = R"({"ok":true, "result": {}})";

#define TELEGRAM_BOT_API_HANDLER_METHOD_SIGNATURE_PARAMS                                           \
    const drogon::HttpRequestPtr &arg_Req, Bot *p_Bot, const std::string &arg_Token,               \
        const drogon::orm::DbClientPtr &p_Db, HttpResponseCallback &&arg_Callback

void handleSendMessage(TELEGRAM_BOT_API_HANDLER_METHOD_SIGNATURE_PARAMS);
void handleSendDocument(TELEGRAM_BOT_API_HANDLER_METHOD_SIGNATURE_PARAMS);
void handleFileQueryById(TELEGRAM_BOT_API_HANDLER_METHOD_SIGNATURE_PARAMS);

#undef TELEGRAM_BOT_API_HANDLER_METHOD_SIGNATURE_PARAMS

} // namespace TelegramBotApi

class Handler {
  private:
    std::filesystem::path db_file_path;
    drogon::orm::DbClientPtr p_db_client;
    std::filesystem::path data_dowload_dir = std::filesystem::current_path() / "data/downloads";
    std::vector<Bot> cached_bots = {};

    // Define the function pointer type first
    using HandlerMethodFuncPtr = void (*)(const drogon::HttpRequestPtr &,
                                          Bot *,
                                          const std::string &,
                                          const drogon::orm::DbClientPtr &,
                                          HttpResponseCallback &&);
    const std::array<std::pair<std::string_view, HandlerMethodFuncPtr>, 3> method_handler_map = {
        {{"sendMessage", TelegramBotApi::handleSendMessage},
         {"sendDocument", TelegramBotApi::handleSendDocument},
         {"getFile", TelegramBotApi::handleFileQueryById}}};

  public:
    drogon::HttpAppFramework &drogon_app;

    /*Default constructors must be deleted*/
    Handler() = delete;
    Handler(const Handler &Other_Handler) = delete;
    Handler(Handler &&Other_Handler) = delete;

    Handler(drogon::HttpAppFramework &Drogon_App);

    Handler &setDbFilePath(const std::filesystem::path &Db_File_Path);
    Handler &initDbClient(void);

    void registerAll();

  protected:
    void registerUploadHandler();
    void registerEchoHandler();

    inline void _dispatchHandlerAccordingToMethod(
        const std::string &Method_Literal,
        const drogon::HttpRequestPtr &arg_Req,
        Bot *p_Bot,
        const std::string &arg_Token,
        drogon::orm::DbClientPtr &p_db_client,
        std::function<void(const drogon::HttpResponsePtr &)> &&arg_Callback) {
        for(const auto &[method_name, handler] : method_handler_map) {
            if(Method_Literal == method_name) {
                handler(arg_Req, p_Bot, arg_Token, p_db_client, std::move(arg_Callback));
                return;
            }
        }
        // Handle unknown method
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k404NotFound);
        const std::string &message = fmt::format("Method not found {}", Method_Literal);
        DEBUG_LOG("{}", message);
        resp->setBody(message);
        arg_Callback(resp);
    }

    void registerTelegramBotApiHandler();
};

} // namespace DrogonRatServer
