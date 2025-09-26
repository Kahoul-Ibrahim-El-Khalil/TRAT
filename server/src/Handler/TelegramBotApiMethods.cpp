#include "DrogonRatServer/Handler.hpp"
#include "DrogonRatServer/debug.hpp"

#include <algorithm>
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/orm/Mapper.h>
#include <fmt/core.h>

namespace DrogonRatServer {
using HttpResponseCallback =
    std::function<void(const drogon::HttpResponsePtr &)>;

// Regex: /bot<token>/<method>
// Capture group 1 = token, group 2 = method name
constexpr char bot_regex_template[] =
    R"(/bot([0-9]+:[A-Za-z0-9_-]+)/([a-zA-Z_]+))";

inline Handler::Bot *_findCachedBot(std::vector<Handler::Bot> &arg_Cache,
                                    const std::string &arg_Token) {
    auto it = std::find_if(
        arg_Cache.begin(), arg_Cache.end(),
        [&](const Handler::Bot &it_Bot) { return it_Bot.token == arg_Token; });
    return (it != arg_Cache.end()) ? &(*it) : nullptr;
}

inline void _insertBotAsync(const drogon::orm::DbClientPtr &p_Db,
                            std::vector<Handler::Bot> &arg_Cache,
                            const std::string &arg_Token) {
    p_Db->execSqlAsync(
        "INSERT INTO telegram_bot (token, can_receive_updates) VALUES (?, 1);",
        [&arg_Cache, arg_Token](const drogon::orm::Result &r) {
            int64_t id = r.insertId();
            arg_Cache.push_back({id, arg_Token});
            DEBUG_LOG("Bot token {} inserted with id {}", arg_Token, id);
        },
        [arg_Token](const drogon::orm::DrogonDbException &err) {
            ERROR_LOG("DB insert failed for token {}: {}", arg_Token,
                      err.base().what());
        },
        arg_Token);

    DEBUG_LOG("Bot token {} cached optimistically (id unknown yet)", arg_Token);
    arg_Cache.push_back({-1, arg_Token});
}

inline void
_handleSendMessage(const drogon::HttpRequestPtr &arg_Req, Handler::Bot *p_Bot,
                   const std::string &arg_Token,
                   const drogon::orm::DbClientPtr &p_Db,
                   HttpResponseCallback &&arg_Callback) {
    auto params = arg_Req->getParameters();
    if(params.find("chat_id") == params.end() ||
       params.find("text") == params.end()) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        resp->setBody("Missing chat_id or text");
        return arg_Callback(resp);
    }

    int chatId = std::stoi(params.at("chat_id"));
    std::string text = params.at("text");

    DEBUG_LOG("sendMessage called with chat_id={} and text=\"{}\"", chatId,
              text);

    if(p_Bot && p_Bot->id > 0) {
        p_Db->execSqlAsync(
            "INSERT INTO telegram_message (bot_id, text) VALUES (?, ?);",
            [p_Bot, text](const drogon::orm::Result &) {
                DEBUG_LOG("Message stored for bot_id={} text={}", p_Bot->id,
                          text);
            },
            [p_Bot, text](const drogon::orm::DrogonDbException &err) {
                ERROR_LOG(
                    "DB insert failed for message bot_id={} text={} err={}",
                    p_Bot->id, text, err.base().what());
            },
            p_Bot->id, text);
    } else {
        ERROR_LOG("sendMessage for token {} but bot_id not yet resolved",
                  arg_Token);
    }

    Json::Value result;
    result["ok"] = true;
    result["result"]["chat"]["id"] = chatId;
    result["result"]["text"] = text;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(result);
    arg_Callback(resp);
}

void Handler::registerTelegramBotApiHandler() {
    this->drogon_app.registerHandlerViaRegex(
        bot_regex_template,
        [this](const drogon::HttpRequestPtr &arg_Req,
               HttpResponseCallback &&arg_Callback,
               const std::string &arg_Token, const std::string &arg_Method) {
            auto p_Bot = _findCachedBot(this->cached_bots, arg_Token);
            if(!p_Bot) {
                _insertBotAsync(p_db_client, this->cached_bots, arg_Token);
                p_Bot = _findCachedBot(this->cached_bots, arg_Token);
            }

            if(arg_Method == "sendMessage") {
                return _handleSendMessage(arg_Req, p_Bot, arg_Token,
                                          p_db_client, std::move(arg_Callback));
            }

            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k404NotFound);
            resp->setBody(fmt::format("Method {} not implemented", arg_Method));
            arg_Callback(resp);
        },
        {drogon::Post, drogon::Get});
}
} // namespace DrogonRatServer
