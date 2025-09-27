#include "DrogonRatServer/Handler.hpp"
#include "DrogonRatServer/debug.hpp"

void DrogonRatServer::TelegramBotApi ::handleSendDocument(
    const drogon::HttpRequestPtr &arg_Req,
    DrogonRatServer::Bot *p_Bot,
    const std::string &arg_Token,
    const drogon::orm::DbClientPtr &p_Db,
    DrogonRatServer::HttpResponseCallback &&arg_Callback) {
    const auto &params = arg_Req->getParameters();
    drogon::MultiPartParser file_upload;

    if(file_upload.parse(arg_Req) != 0 || file_upload.getFiles().empty() ||
       params.find("chat_id") == params.end()) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        Json::Value error;
        error["ok"] = false;
        error["error_code"] = 400;
        error["description"] = "Bad Request: chat_id parameter and document file are required";
        resp->setBody(error.toStyledString());
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        return arg_Callback(resp);
    }

    const auto &files = file_upload.getFiles();
    const int64_t chat_id = std::stoll(params.at("chat_id"));
    std::string caption = params.count("caption") ? params.at("caption") : "";

    int64_t message_id = 0;
    if(p_Bot && p_Bot->id > 0) {
        p_Db->execSqlAsync(
            "INSERT INTO telegram_message (bot_id, chat_id, text) VALUES (?, ?, ?);",
            [&message_id](const drogon::orm::Result &r) { message_id = r.insertId(); },
            [](const drogon::orm::DrogonDbException &err) {
                ERROR_LOG("DB insert failed: {}", err.base().what());
            },
            p_Bot->id,
            chat_id,
            caption.empty() ? "Document" : caption);
    }

    for(const auto &file : files) {
        std::filesystem::path file_path(file.getFileName());
        std::string extension = file_path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

        std::string mime_type = "application/octet-stream";
        if(extension == ".txt")
            mime_type = "text/plain";
        else if(extension == ".mp3")
            mime_type = "audio/mpeg";
        else if(extension == ".ogg")
            mime_type = "audio/ogg";
        else if(extension == ".mp4")
            mime_type = "video/mp4";
        else if(extension == ".pdf")
            mime_type = "application/pdf";
        else if(extension == ".jpg" || extension == ".jpeg")
            mime_type = "image/jpeg";
        else if(extension == ".png")
            mime_type = "image/png";

        if(p_Bot && p_Bot->id > 0) {
            p_Db->execSqlAsync(
                R"(INSERT INTO telegram_file (message_id, file_path, mime_type, extension, data, file_size)
                   VALUES (?, ?, ?, ?, ?, ?);)",
                [file_path](const drogon::orm::Result &) {
                    DEBUG_LOG("Stored file {} successfully", file_path.string());
                },
                [file_path](const drogon::orm::DrogonDbException &err) {
                    ERROR_LOG("DB insert failed for file {}: {}",
                              file_path.string(),
                              err.base().what());
                },
                message_id,
                file_path.string(),
                mime_type,
                extension,
                file.fileData(),
                static_cast<int64_t>(file.fileLength()));
        } else {
            ERROR_LOG("sendDocument for token {} but bot_id not yet resolved", arg_Token);
        }
    }

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k200OK);
    resp->setBody(DrogonRatServer::TelegramBotApi::SUCCESS_JSON_RESPONSE);
    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    arg_Callback(resp);
}
