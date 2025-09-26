#pragma once
#include "DrogonRatServer/Handler.hpp"
#include "DrogonRatServer/debug.hpp"

inline void _handleSendMessage(const drogon::HttpRequestPtr &arg_Req,
                               DrogonRatServer::Handler::Bot *p_Bot,
                               const std::string &arg_Token,
                               const drogon::orm::DbClientPtr &p_Db,
                               DrogonRatServer::HttpResponseCallback &&arg_Callback) {
    auto params = arg_Req->getParameters();
    if(params.find("chat_id") == params.end() || params.find("text") == params.end()) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        resp->setBody("Missing chat_id or text");
        return arg_Callback(resp);
    }

    int chatId = std::stoi(params.at("chat_id"));
    std::string text = params.at("text");

    DEBUG_LOG("sendMessage called with chat_id={} and text=\"{}\"", chatId, text);

    if(p_Bot && p_Bot->id > 0) {
        p_Db->execSqlAsync(
            "INSERT INTO telegram_message (bot_id, text) VALUES (?, ?);",
            [p_Bot, text](const drogon::orm::Result &) {
                DEBUG_LOG("Message stored for bot_id={} text={}", p_Bot->id, text);
            },
            [p_Bot, text](const drogon::orm::DrogonDbException &err) {
                ERROR_LOG("DB insert failed for message bot_id={} text={} err={}",
                          p_Bot->id,
                          text,
                          err.base().what());
            },
            p_Bot->id,
            text);
    } else {
        ERROR_LOG("sendMessage for token {} but bot_id not yet resolved", arg_Token);
    }
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k200OK);
    resp->setBody(DrogonRatServer::TelegramBotApi::SUCCESS_JSON_RESPONSE);
    arg_Callback(resp);
}
