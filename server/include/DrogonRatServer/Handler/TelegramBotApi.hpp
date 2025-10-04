#pragma once

#include "DrogonRatServer/Handler/aliases.hpp"

// Handler signature parameters (can undef later if needed)
#define TELEGRAM_BOT_API_HANDLER_METHOD_SIGNATURE_PARAMS                                           \
    const drogon::HttpRequestPtr &arg_Req, Bot *p_Bot, const std::string &arg_Token,               \
        const drogon::orm::DbClientPtr &p_Db, HttpResponseCallback &&arg_Callback

#define getUpdate_FUNCTION_SIGNATURE                                                               \
    DrogonRatServer::Bot *p_Bot, const std::string &arg_Token,                                     \
        const drogon::orm::DbClientPtr &p_Db,                                                      \
        DrogonRatServer::HttpResponseCallback &&arg_Callback,                                      \
        DrogonRatServer::Handler &Caller_Handler

// Main TelegramBotApi macro
#define TELEGRAM_BOT_API_MACRO                                                                     \
    namespace DrogonRatServer {                                                                    \
    class Handler;                                                                                 \
    struct Bot {                                                                                   \
        int64_t id;                                                                                \
        std::string token;                                                                         \
    };                                                                                             \
    namespace TelegramBotApi {                                                                     \
    constexpr char SUCCESS_JSON_RESPONSE[] = R"({"ok":true, "result": {}})";                       \
    void handleSendMessage(TELEGRAM_BOT_API_HANDLER_METHOD_SIGNATURE_PARAMS);                      \
    void handleSendDocument(TELEGRAM_BOT_API_HANDLER_METHOD_SIGNATURE_PARAMS);                     \
    void handleGetUpdate(getUpdate_FUNCTION_SIGNATURE);                                            \
    void handleFileQueryById(TELEGRAM_BOT_API_HANDLER_METHOD_SIGNATURE_PARAMS);                    \
    using HandlerMethodFuncPtr = void (*)(const drogon::HttpRequestPtr &,                          \
                                          Bot *,                                                   \
                                          const std::string &,                                     \
                                          const drogon::orm::DbClientPtr &,                        \
                                          HttpResponseCallback &&);                                \
    } /* namespace TelegramBotApi */                                                               \
    } /* namespace DrogonRatServer */

// Routing map macro
#define TELEGRAM_BOT_API_METHOD_ROUTING_MAP                                                        \
  protected:                                                                                       \
    const std::array<                                                                              \
        std::pair<std::string_view, DrogonRatServer::TelegramBotApi::HandlerMethodFuncPtr>,        \
        3>                                                                                         \
        method_handler_map = {                                                                     \
            {{"sendMessage", DrogonRatServer::TelegramBotApi::handleSendMessage},                  \
             {"sendDocument", DrogonRatServer::TelegramBotApi::handleSendDocument},                \
             {"getFile", DrogonRatServer::TelegramBotApi::handleFileQueryById}}};
