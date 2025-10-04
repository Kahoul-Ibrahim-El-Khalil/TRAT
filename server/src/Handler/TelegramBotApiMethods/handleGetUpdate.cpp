#include "DrogonRatServer/Handler.hpp"

#include <drogon/HttpResponse.h>
#include <mutex>

#define getUpdate_FUNCTION_SIGNATURE                                                               \
    DrogonRatServer::Bot *p_Bot, const std::string &arg_Token,                                     \
        const drogon::orm::DbClientPtr &p_Db,                                                      \
        DrogonRatServer::HttpResponseCallback &&arg_Callback,                                      \
        DrogonRatServer::Handler &Caller_Handler

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

#define SEND_EMPTY_UPDATE_RESPONSE(arg_Callback)                                                   \
    do {                                                                                           \
        Json::Value response;                                                                      \
        response["ok"] = true;                                                                     \
        response["result"] = Json::Value(Json::arrayValue);                                        \
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);                           \
        resp->setStatusCode(drogon::k200OK);                                                       \
        (arg_Callback)(resp);                                                                      \
        return;                                                                                    \
    } while(0)

bool inline _insertUpdateInDb(const int64_t Bot_Id,
                              const drogon::orm::DbClientPtr &p_Db,
                              const Json::Value &update_json) {
    try {
        // const int64_t update_id = update_json["update_id"].asInt64();
        const Json::Value &message = update_json["message"];
        const int64_t message_id = message["message_id"].asInt64();
        const std::string &caption = message.get("caption", "").asString();
        const std::string &text = message.get("text", "").asString();

        // Insert message to telegram_message table
        p_Db->execSqlSync("INSERT INTO telegram_message(bot_id, text, caption) VALUES(?, ?, ?)",
                          Bot_Id,
                          text,
                          caption);

        // Insert into telegram_update table
        p_Db->execSqlSync(
            "INSERT INTO telegram_update(bot_id, message_id, delivered) VALUES(?, ?, ?)",
            Bot_Id,
            message_id,
            true // Mark as delivered since we're sending it
        );

        return true;
    } catch(const std::exception &e) {
        FILE_ERROR_LOG("Failed to insert update into database: {}", e.what());
        return false;
    }
}

void DrogonRatServer::TelegramBotApi::handleGetUpdate(getUpdate_FUNCTION_SIGNATURE) {
    if(!p_Bot || p_Bot->id <= 1) {
        FILE_ERROR_LOG("getUpdates called for token {} but bot_id not yet resolved", arg_Token);
        SEND_ERROR_RESPONSE(arg_Callback, drogon::k401Unauthorized, "Bot not initialized");
        return;
    }

    // since I am the one who built the client, I will simply ignore offset  and limit and impliment
    // my own custom behavior;

    std::shared_ptr<DrogonRatServer::Handler::Shell> &shell_sptr = Caller_Handler.getShell();
    std::deque<Json::Value> &updates_queue = shell_sptr->getPendingUpdatesQueue();
    std::mutex &updates_queue_mutex = shell_sptr->getPendingUpdatesMutex();

    Json::Value update;

    updates_queue_mutex.lock();
    try {
        if(updates_queue.empty()) {
            SEND_EMPTY_UPDATE_RESPONSE(arg_Callback);
            return;
        }

        while(!updates_queue.empty()) {
            update = std::move(updates_queue.front());
            updates_queue.pop_front();
        }

        _insertUpdateInDb(p_Bot->id, p_Db, update);
        SEND_SUCCESS_RESPONSE(arg_Callback, update);

    } catch(const std::exception &e) {
        FILE_ERROR_LOG("Error processing updates: {}", e.what());
        SEND_ERROR_RESPONSE(arg_Callback,
                            drogon::k500InternalServerError,
                            fmt::format("Error processing updates: {}", +e.what()));
    }
    updates_queue_mutex.unlock();
}

#undef getUpdate_FUNCTION_SIGNATURE
#undef SEND_SUCCESS_RESPONSE
#undef SEND_ERROR_RESPONSE
#undef SEND_EMPTY_UPDATE_RESPONSE
