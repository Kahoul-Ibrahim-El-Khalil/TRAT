
#include "DrogonRatServer/Handler.hpp"

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/orm/Result.h>
#include <filesystem>
#include <fmt/core.h>
#include <memory>
#include <optional>

/*
    CREATE TABLE IF NOT EXISTS telegram_file (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        message_id INTEGER NOT NULL REFERENCES telegram_message(id) ON DELETE CASCADE,
        file_id INTEGER NOT NULL REFERENCES file(id),
        original_file_path TEXT,
        mime_type TEXT
    );
*/

// ============================================================================
// Utility Macros
// ============================================================================

#define SEND_ERROR_RESPONSE(sptr_Callback, arg_Status, arg_Message)                                \
    do {                                                                                           \
        Json::Value error;                                                                         \
        error["ok"] = false;                                                                       \
        error["description"] = (arg_Message);                                                      \
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);                              \
        resp->setStatusCode((arg_Status));                                                         \
        (*sptr_Callback)(resp);                                                                    \
    } while(0)

#define SEND_SUCCESS_RESPONSE(sptr_Callback, Result_Json)                                          \
    do {                                                                                           \
        Json::Value response;                                                                      \
        response["ok"] = true;                                                                     \
        response["result"] = (Result_Json);                                                        \
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);                           \
        resp->setStatusCode(drogon::k200OK);                                                       \
        (*sptr_Callback)(resp);                                                                    \
    } while(0)

#define HANDLE_DB_ERROR(sptr_Callback, p_Bot, File_Id, arg_Err)                                    \
    do {                                                                                           \
        FILE_ERROR_LOG("DB query failed for bot_id={} file_id={} err={}",                          \
                       (p_Bot)->id,                                                                \
                       (File_Id),                                                                  \
                       (arg_Err).base().what());                                                   \
        SEND_ERROR_RESPONSE((sptr_Callback), drogon::k500InternalServerError, "Database error");   \
    } while(0)

inline std::optional<int>
_getFileIdFromHttpRequest(const drogon::HttpRequestPtr &arg_Req,
                          DrogonRatServer::HttpResponseCallbackPtr &sptr_Callback) {
    const auto &params = arg_Req->getParameters();

    auto it = params.find("file_id");
    if(it == params.end()) {
        SEND_ERROR_RESPONSE(sptr_Callback,
                            drogon::k400BadRequest,
                            "Missing required parameter 'file_id'");
        return std::nullopt;
    }

    try {
        return std::stoi(it->second);
    } catch(...) {
        SEND_ERROR_RESPONSE(sptr_Callback,
                            drogon::k400BadRequest,
                            "Invalid 'file_id' parameter (must be integer)");
        return std::nullopt;
    }
}

inline void _handleNonResolvedToken(DrogonRatServer::Bot *p_Bot,
                                    const std::string &arg_Token,
                                    DrogonRatServer::HttpResponseCallbackPtr &sptr_Callback) {
    if(!p_Bot) {
        FILE_ERROR_LOG("getFile for token {} but bot_id not yet resolved", arg_Token);
        SEND_ERROR_RESPONSE(sptr_Callback, drogon::k400BadRequest, "Bot not initialized");
    }
}

void DrogonRatServer::TelegramBotApi::handleFileQueryById(const drogon::HttpRequestPtr &arg_Req,
                                                          Bot *p_Bot,
                                                          const std::string &arg_Token,
                                                          const drogon::orm::DbClientPtr &p_Db,
                                                          HttpResponseCallback &&arg_Callback) {
    auto callback_sptr =
        std::make_shared<DrogonRatServer::HttpResponseCallback>(std::move(arg_Callback));

    const std::optional<int> &file_id_opt = _getFileIdFromHttpRequest(arg_Req, callback_sptr);
    if(!file_id_opt.has_value()) {
        FILE_DEBUG_LOG("Invalid or missing file id in the received HTTP request: {}",
                       arg_Req->getBody());
        return;
    }
    const int &file_id = file_id_opt.value();
    _handleNonResolvedToken(p_Bot, arg_Token, callback_sptr);

    auto success_lambda = [callback_sptr, file_id](const drogon::orm::Result &res) {
        if(res.empty()) {
            SEND_ERROR_RESPONSE(callback_sptr,
                                drogon::k404NotFound,
                                fmt::format("File with id = {} not found", file_id));
        }

        const auto &row = res[0];
        const std::string &file_path = row["path"].as<std::string>();

        Json::Value result;
        result["file_id"] = std::to_string(file_id);
        result["file_size"] = std::filesystem::file_size(file_path);
        result["file_path"] = std::move(file_path);

        SEND_SUCCESS_RESPONSE(callback_sptr, result);
    };

    auto failure_lambda =
        [callback_sptr, p_Bot, file_id](const drogon::orm::DrogonDbException &arg_Err) {
            HANDLE_DB_ERROR(callback_sptr, p_Bot, file_id, arg_Err);
        };

    // Execute query asynchronously
    p_Db->execSqlAsync("SELECT id, path, created_at FROM file WHERE file.id = ?",
                       std::move(success_lambda),
                       std::move(failure_lambda),
                       file_id);
}

#undef SEND_ERROR_RESPONSE
#undef SEND_SUCCESS_RESPONSE
#undef HANDLE_DB_ERROR
