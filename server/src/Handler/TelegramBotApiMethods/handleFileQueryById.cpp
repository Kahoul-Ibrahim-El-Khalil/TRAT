#include "DrogonRatServer/Handler.hpp"

/*
    CREATE TABLE IF NOT EXISTS telegram_file (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        message_id INTEGER NOT NULL REFERENCES telegram_message(id) ON DELETE CASCADE,
        file_id INTEGER NOT NULL REFERENCES file(id),
        original_file_path TEXT,
        mime_type TEXT
    );


*/
// fmt::format("{}/bot{}/getFile?file_id=", this->server_api_url, token);

void DrogonRatServer::TelegramBotApi::handleFileQueryById(const drogon::HttpRequestPtr &arg_Req,
                                                          Bot *p_Bot,
                                                          const std::string &arg_Token,
                                                          const drogon::orm::DbClientPtr &p_Db,
                                                          HttpResponseCallback &&arg_Callback) {
    const auto &params = arg_Req->getParameters();

    // Check for file_id param
    auto it = params.find("file_id");
    if(it == params.end()) {
        Json::Value error;
        error["ok"] = false;
        error["description"] = "Missing required parameter 'file_id'";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        return arg_Callback(resp);
    }

    int file_id = 0;
    try {
        file_id = std::stoi(it->second);
    } catch(...) {
        Json::Value error;
        error["ok"] = false;
        error["description"] = "Invalid 'file_id' parameter (must be integer)";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        return arg_Callback(resp);
    }

    if(!p_Bot) {
        ERROR_LOG("getFile for token {} but bot_id not yet resolved", arg_Token);
        Json::Value error;
        error["ok"] = false;
        error["description"] = "Bot not initialized";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        return arg_Callback(resp);
    }

    // Query the file data from your DB
    p_Db->execSqlAsync(
        "SELECT id, original_file_path, mime_type FROM telegram_file WHERE file_id = ?;",
        [arg_Callback, file_id, arg_Token](const drogon::orm::Result &r) {
            if(r.empty()) {
                Json::Value error;
                error["ok"] = false;
                error["description"] = "File not found";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
                resp->setStatusCode(drogon::k404NotFound);
                return arg_Callback(resp);
            }

            const auto &row = r[0];
            std::string file_path = row["original_file_path"].as<std::string>();
            std::string mime_type = row["mime_type"].as<std::string>();

            // Build Telegram-style JSON
            Json::Value result;
            result["file_id"] = std::to_string(file_id);
            result["file_unique_id"] = fmt::format("unique_{}", file_id); // optional placeholder
            result["mime_type"] = mime_type;
            result["file_path"] = file_path;

            Json::Value response;
            response["ok"] = true;
            response["result"] = result;

            auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
            resp->setStatusCode(drogon::k200OK);
            arg_Callback(resp);
        },
        [arg_Callback, p_Bot, file_id](const drogon::orm::DrogonDbException &err) {
            ERROR_LOG("DB query failed for bot_id={} file_id={} err={}",
                      p_Bot->id,
                      file_id,
                      err.base().what());
            Json::Value error;
            error["ok"] = false;
            error["description"] = "Database error";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k500InternalServerError);
            arg_Callback(resp);
        },
        file_id);
}
