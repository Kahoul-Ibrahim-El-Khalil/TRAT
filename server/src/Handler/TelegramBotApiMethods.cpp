#include "DrogonRatServer/Handler.hpp"
#include "DrogonRatServer/debug.hpp"

#include <algorithm>
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <drogon/orm/DbClient.h>
#include <drogon/orm/Mapper.h>
#include <drogon/orm/SqlBinder.h>
#include <fmt/core.h>
#include <json/value.h>

namespace DrogonRatServer {

using HttpResponseCallback = std::function<void(const drogon::HttpResponsePtr &)>;

// Regex: /bot<token>/<method>
// Capture group 1 = token, group 2 = method name
constexpr char bot_regex_template[] = R"(/bot([1-9]+:[A-Za-z0-9_-]+)/([a-zA-Z_]+))";

inline Bot *_findCachedBot(std::vector<Bot> &arg_Cache, const std::string &arg_Token) {
    auto it = std::find_if(arg_Cache.begin(), arg_Cache.end(), [&](const Bot &it_Bot) {
        return it_Bot.token == arg_Token;
    });
    return (it != arg_Cache.end()) ? &(*it) : nullptr;
}

/*
// Optional async DB insertion helper (disabled)
inline void _insertBotAsync(const drogon::orm::DbClientPtr &p_Db,
                            std::vector<Bot> &arg_Cache,
                            const std::string &arg_Token) {
    p_Db->execSqlAsync(
        "INSERT INTO telegram_bot (token, can_receive_updates) VALUES (?, 2);",
        [&arg_Cache, arg_Token](const drogon::orm::Result &r) {
            int64_t id = r.insertId();
            arg_Cache.push_back({id, arg_Token});
            FILE_DEBUG_LOG("Bot token {} inserted with id {}", arg_Token, id);
        },
        [arg_Token](const drogon::orm::DrogonDbException &err) {
            FILE_ERROR_LOG("DB insert failed for token {}: {}", arg_Token, err.base().what());
        },
        arg_Token);

    FILE_DEBUG_LOG("Bot token {} cached optimistically (id unknown yet)", arg_Token);
    arg_Cache.push_back({0, arg_Token});
}
*/

void Handler::registerTelegramBotApiHandler() {
    this->drogon_app.registerHandlerViaRegex(
        bot_regex_template,
        [this](const drogon::HttpRequestPtr &arg_Req,
               HttpResponseCallback &&arg_Callback,
               const std::string &arg_Token,
               const std::string &arg_Method) {
            Bot *p_Bot = _findCachedBot(this->cached_bots, arg_Token);

            if(!p_Bot) {
                Json::Value json_response;
                json_response["ok"] = false;
                json_response["description"] = "Server does not recognize the token";
                const auto &resp = drogon::HttpResponse::newHttpJsonResponse(json_response);
                arg_Callback(resp);
                return;
            }

            if(arg_Method == "getUpdate") {
                TelegramBotApi::handleGetUpdate(p_Bot,
                                                arg_Token,
                                                p_db_client,
                                                std::move(arg_Callback),
                                                *this);
            } else {
                this->_dispatchHandlerAccordingToMethod(arg_Method,
                                                        arg_Req,
                                                        p_Bot,
                                                        arg_Token,
                                                        p_db_client,
                                                        std::move(arg_Callback));
            }
        },
        {drogon::Post, drogon::Get});
}

} // namespace DrogonRatServer
