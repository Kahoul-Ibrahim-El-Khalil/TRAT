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
#include <json/value.h>
#include <memory>
#include <process.hpp> //tiny-process-library
#include <string_view>

namespace DrogonRatServer {
struct Bot {
    int64_t id;
    std::string token;
};

class Handler;

using DrogonHandlerCallback = std::function<void(const drogon::HttpResponsePtr &)>;
using HttpResponseCallback = std::function<void(const drogon::HttpResponsePtr &)>;
using HttpResponseCallbackPtr = std::shared_ptr<HttpResponseCallback>;

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

    int64_t chat_id = 1202032932;
    std::unique_ptr<TinyProcessLib::Process> shell_process = nullptr;
    // In your Handler class definition:
    std::deque<std::string> pending_updates_buffers;
    std::mutex pending_updates_buffers_mutex;

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
    // this is a reference to the singelton, this class does not own it.
    drogon::HttpAppFramework &drogon_app;

    /*Default constructors must be deleted*/
    Handler() = delete;
    Handler(const Handler &Other_Handler) = delete;
    Handler(Handler &&Other_Handler) = delete;

    Handler(drogon::HttpAppFramework &Drogon_App);

    Handler &setDbFilePath(const std::filesystem::path &Db_File_Path);
    Handler &initDbClient(void);

    Handler &launchShellConsole(const std::filesystem::path &Shell_Binary_Path);
    void registerAll();

  protected:
    void registerUploadHandler();
    void registerEchoHandler();

    int64_t queryUpdateOffset();
    int64_t queryMessageOffset();

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
