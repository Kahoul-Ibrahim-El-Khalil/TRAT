#include "DrogonRatServer/Handler.hpp"
#include "DrogonRatServer/debug.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <random>
#include <utility>

// ------------------------------
// Utility Macros
// ------------------------------

#define SEND_SUCCESS_HTTP_RESPONSE(arg_Status, arg_Http_Body, arg_Callback)                        \
    do {                                                                                           \
        auto resp = drogon::HttpResponse::newHttpResponse();                                       \
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(arg_Status));                      \
        resp->setBody(arg_Http_Body);                                                              \
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);                                     \
        arg_Callback(resp);                                                                        \
        return;                                                                                    \
    } while(0)

#define SEND_JSON_ERROR_RESPONSE(arg_Callback, arg_Code, arg_Msg)                                  \
    do {                                                                                           \
        auto resp = drogon::HttpResponse::newHttpResponse();                                       \
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(arg_Code));                        \
        Json::Value error;                                                                         \
        error["ok"] = false;                                                                       \
        error["error_code"] = arg_Code;                                                            \
        error["description"] = arg_Msg;                                                            \
        resp->setBody(error.toStyledString());                                                     \
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);                                     \
        arg_Callback(resp);                                                                        \
        return;                                                                                    \
    } while(0)

// ------------------------------
// Utility Functions
// ------------------------------

inline std::string determineMimeType(const std::string &arg_Extension) {
    static const std::pair<const char *, const char *> mime_list[] = {
        {".txt", "text/plain"},
        {".mp3", "audio/mpeg"},
        {".ogg", "audio/ogg"},
        {".mp4", "video/mp4"},
        {".pdf", "application/pdf"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".png", "image/png"},
    };

    for(const auto &p : mime_list) {
        if(arg_Extension == p.first)
            return p.second;
    }

    return "application/octet-stream";
}

inline std::string generateSafeFilename(const std::string &arg_Original) {
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    static std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<uint64_t> dist(1000, 9999);

    std::filesystem::path path(arg_Original);
    std::string base = path.stem().string();
    std::string ext = path.extension().string();

    return fmt::format("{}_{}_{}{}", base, now, dist(rng), ext);
}

// ------------------------------
// Handler Implementation
// ------------------------------

void DrogonRatServer::TelegramBotApi::handleSendDocument(
    const drogon::HttpRequestPtr &arg_Req,
    DrogonRatServer::Bot *p_Bot,
    const std::string &arg_Token,
    const drogon::orm::DbClientPtr &p_Db,
    DrogonRatServer::HttpResponseCallback &&arg_Callback) {
    drogon::MultiPartParser file_upload;

    DEBUG_LOG("sendDocument received from {}", arg_Token);

    if(file_upload.parse(arg_Req) != 0) {
        SEND_JSON_ERROR_RESPONSE(arg_Callback, 400, "Bad Request: failed to parse multipart data");
    }

    if(file_upload.getFiles().empty()) {
        SEND_JSON_ERROR_RESPONSE(arg_Callback, 400, "Bad Request: document file is required");
    }

    std::string chat_id_str;
    std::string caption;

    for(const auto &item : file_upload.getParameters()) {
        if(item.first == "chat_id")
            chat_id_str = item.second;
        else if(item.first == "caption")
            caption = item.second;
    }

    if(chat_id_str.empty()) {
        SEND_JSON_ERROR_RESPONSE(arg_Callback, 400, "Bad Request: chat_id parameter is required");
    }

    const int64_t chat_id = std::stoll(chat_id_str);
    if(!p_Bot || p_Bot->id <= 0) {
        SEND_JSON_ERROR_RESPONSE(arg_Callback, 401, "Bot not initialized or bot_id invalid");
    }

    const auto &files = file_upload.getFiles();
    const std::string message_text = caption.empty() ? "Document" : caption;

    // Insert telegram_message record
    p_Db->execSqlAsync(
        "INSERT INTO telegram_message (bot_id, chat_id, text) VALUES (?, ?, ?);",
        [p_Db, files, message_text, arg_Callback](const drogon::orm::Result &r) mutable {
            int64_t message_id = 0;

            try {
                message_id = r.insertId();
            } catch(const std::exception &e) {
                ERROR_LOG("Failed to get insert ID: {}", e.what());
                SEND_JSON_ERROR_RESPONSE(arg_Callback,
                                         500,
                                         "Database error: failed to get message ID");
                return;
            }

            auto files_remaining = std::make_shared<std::atomic<size_t>>(files.size());

            for(const auto &file : files) {
                std::filesystem::path safe_path =
                    fmt::format("{}/downloads/{}",
                                std::filesystem::current_path().string(),
                                generateSafeFilename(file.getFileName()));

                try {
                    file.save(safe_path.string());
                } catch(const std::exception &e) {
                    ERROR_LOG("Failed to save file {}: {}", safe_path.string(), e.what());
                    if(--(*files_remaining) == 0)
                        SEND_JSON_ERROR_RESPONSE(arg_Callback,
                                                 500,
                                                 "Failed to save one or more files");
                    continue;
                }

                std::string extension = safe_path.extension().string();
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                std::string mime_type = determineMimeType(extension);

                // Step 1: Insert into file() table
                p_Db->execSqlAsync(
                    "INSERT INTO file (path) VALUES (?);",
                    [p_Db, safe_path, mime_type, message_id, files_remaining, arg_Callback](
                        const drogon::orm::Result &res) mutable {
                        int64_t file_id = 0;
                        try {
                            file_id = res.insertId();
                        } catch(const std::exception &e) {
                            ERROR_LOG("Failed to get file.id: {}", e.what());
                        }

                        // Step 2: Link in telegram_file
                        p_Db->execSqlAsync(
                            "INSERT INTO telegram_file (message_id, file_id, original_file_path, "
                            "mime_type) VALUES (?, ?, ?, ?);",
                            [files_remaining, safe_path, arg_Callback](
                                const drogon::orm::Result &) {
                                DEBUG_LOG("Linked file {} successfully", safe_path.string());
                                if(--(*files_remaining) == 0)
                                    SEND_SUCCESS_HTTP_RESPONSE(
                                        drogon::k201Created,
                                        DrogonRatServer::TelegramBotApi::SUCCESS_JSON_RESPONSE,
                                        arg_Callback);
                            },
                            [files_remaining, safe_path, arg_Callback](
                                const drogon::orm::DrogonDbException &err) {
                                ERROR_LOG("DB insert failed for telegram_file {}: {}",
                                          safe_path.string(),
                                          err.base().what());
                                if(--(*files_remaining) == 0)
                                    SEND_JSON_ERROR_RESPONSE(
                                        arg_Callback,
                                        500,
                                        "Database error: failed to link files");
                            },
                            message_id,
                            file_id,
                            safe_path.string(),
                            mime_type);
                    },
                    [files_remaining, safe_path, arg_Callback](
                        const drogon::orm::DrogonDbException &err) {
                        ERROR_LOG("DB insert failed for file {}: {}",
                                  safe_path.string(),
                                  err.base().what());
                        if(--(*files_remaining) == 0)
                            SEND_JSON_ERROR_RESPONSE(arg_Callback,
                                                     500,
                                                     "Database error: failed to store file record");
                    },
                    safe_path.string());
            }
        },
        [arg_Callback](const drogon::orm::DrogonDbException &err) {
            ERROR_LOG("DB insert failed (telegram_message): {}", err.base().what());
            SEND_JSON_ERROR_RESPONSE(arg_Callback,
                                     500,
                                     "Database error: failed to create message record");
        },
        p_Bot->id,
        chat_id,
        message_text);
}
