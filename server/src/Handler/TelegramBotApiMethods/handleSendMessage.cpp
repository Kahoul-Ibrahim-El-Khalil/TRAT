#include "DrogonRatServer/Handler.hpp"
#include "DrogonRatServer/debug.hpp"

#include <memory>
#define SEND_ERROR_RESPONSE(arg_Callback, arg_Status, arg_Message)                                 \
    do {                                                                                           \
        Json::Value error;                                                                         \
        error["ok"] = false;                                                                       \
        error["description"] = (arg_Message);                                                      \
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);                              \
        resp->setStatusCode((arg_Status));                                                         \
        (arg_Callback)(resp);                                                                      \
        return;                                                                                    \
    } while(0)

#define SEND_SUCCESS_RESPONSE(arg_Callback, Result_Json)                                           \
    do {                                                                                           \
        Json::Value response;                                                                      \
        response["ok"] = true;                                                                     \
        response["result"] = (Result_Json);                                                        \
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);                           \
        resp->setStatusCode(drogon::k200OK);                                                       \
        (arg_Callback)(resp);                                                                      \
        return;                                                                                    \
    } while(0)

#define SEND_PLAIN_RESPONSE(arg_Callback, arg_Status, arg_Text)                                    \
    do {                                                                                           \
        auto resp = drogon::HttpResponse::newHttpResponse();                                       \
        resp->setStatusCode((arg_Status));                                                         \
        resp->setBody((arg_Text));                                                                 \
        (arg_Callback)(resp);                                                                      \
        return;                                                                                    \
    } while(0)

void DrogonRatServer::TelegramBotApi::handleSendMessage(
    const drogon::HttpRequestPtr &arg_Req,
    DrogonRatServer::Bot *p_Bot,
    const std::string &arg_Token,
    const drogon::orm::DbClientPtr &p_Db,
    DrogonRatServer::HttpResponseCallback &&arg_Callback) {
    const auto &params = arg_Req->getParameters();

    if(params.find("chat_id") == params.end() || params.find("text") == params.end()) {
        SEND_ERROR_RESPONSE(arg_Callback, drogon::k400BadRequest, "Missing chat_id or text");
    }

    int chat_id = 0;
    try {
        chat_id = std::stoi(params.at("chat_id"));
    } catch(...) {
        SEND_ERROR_RESPONSE(arg_Callback,
                            drogon::k400BadRequest,
                            "Invalid 'chat_id' parameter (must be integer)");
    }

    const std::string &text = params.at("text");
    FILE_DEBUG_LOG("sendMessage called with chat_id={} and text=\"{}\"", chat_id, text);

    // Check bot validity
    if(!p_Bot || p_Bot->id <= 0) {
        FILE_ERROR_LOG("sendMessage for token {} but bot_id not yet resolved", arg_Token);
        SEND_ERROR_RESPONSE(arg_Callback, drogon::k400BadRequest, "Bot not initialized");
    }

    static constexpr const char *SQL_INSERT =
        "INSERT INTO telegram_message (bot_id, text) VALUES (?, ?);";

#ifdef DEBUG
    auto text_sptr = std::make_shared<std::string>(text);

    auto success_lambda = [p_Bot, text_sptr, chat_id](const drogon::orm::Result &) {
        FILE_DEBUG_LOG("Message stored for bot_id={} chat_id={} text={}",
                       p_Bot->id,
                       chat_id,
                       *text_sptr);
    };

    auto failure_lambda = [p_Bot, text_sptr](const drogon::orm::DrogonDbException &arg_Err) {
        FILE_ERROR_LOG("DB insert failed for message bot_id={} text={} err={}",
                       p_Bot->id,
                       *text_sptr,
                       arg_Err.base().what());
    };

    p_Db->execSqlAsync(SQL_INSERT,
                       std::move(success_lambda),
                       std::move(failure_lambda),
                       p_Bot->id,
                       *text_sptr);
#else
    p_Db->execSqlAsync(SQL_INSERT, nullptr, nullptr, p_Bot->id, text);
#endif

    // Build result JSON
    Json::Value result;
    result["chat_id"] = chat_id;
    result["text"] = text;
    result["status"] = "queued";

    SEND_SUCCESS_RESPONSE(arg_Callback, result);
}

#undef SEND_SUCCESS_RESPONSE
#undef SEND_ERROR_RESPONSE
#undef SEND_PLAIN_RESPONSE
