/*include/DrogonRatServer/Handler.hpp*/
#pragma once
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>
#include <drogon/orm/Mapper.h>
#include <functional>

namespace DrogonRatServer {
using DrogonHandlerCallback = std::function<void(const drogon::HttpResponsePtr &)>;
using HttpResponseCallback = std::function<void(const drogon::HttpResponsePtr &)>;

class Handler {
  public:
    struct Bot {
        int64_t id;
        std::string token;
    };

  private:
    std::filesystem::path db_file_path;
    drogon::orm::DbClientPtr p_db_client;

    std::vector<Bot> cached_bots = {};

  public:
    drogon::HttpAppFramework &drogon_app; // The parent class of this calls it,
    /*Default constructors must be deleted*/
    Handler() = delete;
    Handler(const Handler &Other_Handler) = delete;
    Handler(Handler &&Other_Handler) = delete;

    Handler(drogon::HttpAppFramework &Drogon_App);

    Handler &setDbFilePath(const std::filesystem::path &Db_File_Path);
    Handler &initDbClient(void);

    void registerAll();
    //  Implimentation in src/Handler/methods.cpp
  protected:
    void registerUploadHandler();
    void registerEchoHandler();

    void registerSendDocumentHandler();
    void registerTelegramBotApiHandler();
};

namespace TelegramBotApi {
constexpr char SUCCESS_JSON_RESPONSE[] = R"({{"ok ":true, " result ": {}})";

} // namespace TelegramBotApi
} // namespace DrogonRatServer

#undef HANDLER_ROUTE_MAP
