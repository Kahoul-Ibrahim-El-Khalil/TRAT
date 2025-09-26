#include "DrogonRatServer/Handler.hpp"
#include "DrogonRatServer/debug.hpp"
// inlined functions prefixed with _
#include "TelegramBotApiMethods/handleSendDocument.hpp"
#include "TelegramBotApiMethods/handleSendMessage.hpp"

#include <algorithm>
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/orm/Mapper.h>
#include <drogon/orm/SqlBinder.h>
#include <fmt/core.h>

namespace DrogonRatServer {
using HttpResponseCallback = std::function<void(const drogon::HttpResponsePtr &)>;

// Regex: /bot<token>/<method>
// Capture group 1 = token, group 2 = method name
constexpr char bot_regex_template[] = R"(/bot([0-9]+:[A-Za-z0-9_-]+)/([a-zA-Z_]+))";

inline Handler::Bot *
_findCachedBot(std::vector<Handler::Bot> &arg_Cache, const std::string &arg_Token) {
    auto it = std::find_if(arg_Cache.begin(), arg_Cache.end(), [&](const Handler::Bot &it_Bot) {
        return it_Bot.token == arg_Token;
    });
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
            ERROR_LOG("DB insert failed for token {}: {}", arg_Token, err.base().what());
        },
        arg_Token);

    DEBUG_LOG("Bot token {} cached optimistically (id unknown yet)", arg_Token);
    arg_Cache.push_back({-1, arg_Token});
}

void Handler::registerTelegramBotApiHandler() {
    this->drogon_app.registerHandlerViaRegex(
        bot_regex_template,
        [this](const drogon::HttpRequestPtr &arg_Req,
               HttpResponseCallback &&arg_Callback,
               const std::string &arg_Token,
               const std::string &arg_Method) {
            auto p_Bot = _findCachedBot(this->cached_bots, arg_Token);
            if(!p_Bot) {
                _insertBotAsync(p_db_client, this->cached_bots, arg_Token);
                p_Bot = _findCachedBot(this->cached_bots, arg_Token);
            }

            if(arg_Method == "sendMessage") {
                _handleSendMessage(arg_Req, p_Bot, arg_Token, p_db_client, std::move(arg_Callback));
            } else if(arg_Method == "sendDocument") {
                _handleSendDocument(arg_Req, p_Bot, arg_Token, p_db_client, std::move(arg_Callback));
            }

            // Handle unknown methods with proper Telegram API error response
            Json::Value error;
            error["ok"] = false;
            error["error_code"] = 404;
            error["description"] = fmt::format("Not Found: method {} not found", arg_Method);

            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k404NotFound);
            arg_Callback(resp);
        },
        {drogon::Post, drogon::Get});
}
} // namespace DrogonRatServer
