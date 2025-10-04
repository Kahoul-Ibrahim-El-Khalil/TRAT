/*server/include/DrogonRatServer/Handler.hpp*/

#pragma once
#include "DrogonRatServer/Handler/Shell.hpp"
#include "DrogonRatServer/Handler/TelegramBotApi.hpp"
#include "DrogonRatServer/Handler/inlinedDispatcher.hpp"
#include "DrogonRatServer/debug.hpp"

#include <array>
#include <deque>
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>
#include <drogon/orm/Mapper.h>
#include <filesystem>
#include <fmt/core.h>
#include <functional>
#include <json/value.h>
#include <memory> //for std::shared_ptr
#include <mutex>  //for thread safety in DrogonRatServer::Handler::Shell
#include <string_view>

/*Defined in DrogonRatServer/Handler/TelegramBotApi.hpp*/
TELEGRAM_BOT_API_MACRO

namespace DrogonRatServer {

class Handler {
  private:
    std::filesystem::path db_file_path;
    drogon::orm::DbClientPtr p_db_client;
    std::filesystem::path data_dowload_dir = std::filesystem::current_path() / "data/downloads";
    std::vector<Bot> cached_bots = {};

  public:
    drogon::HttpAppFramework &drogon_app;

    Handler() = delete;

    Handler(const Handler &) = delete;
    Handler &operator=(const Handler &Other_Handler) = delete;

    Handler(Handler &&) = delete;
    Handler &&operator=(Handler &&Other_Handler) = delete;

    Handler(drogon::HttpAppFramework &Drogon_App);
    ~Handler() = default;

    Handler &setDbFilePath(const std::filesystem::path &Db_File_Path);
    Handler &initDbClient(void);
    Handler &initShell(const int64_t &Server_Id);

    void registerAll();
    void registerUploadHandler();
    void registerEchoHandler();
    void registerTelegramBotApiHandler();

    int64_t queryUpdateOffset();
    int64_t queryMessageOffset();
    int64_t queryFileOffset();
    /*Defined in DrogonRatServer/Handler/Shell.hpp*/
    SHELL_INLINED_MACRO
    /*Defined in DrogonRatServer/Handler/TelegramBotApi.hpp*/
    TELEGRAM_BOT_API_METHOD_ROUTING_MAP
    /*Defined in DrogonRatServer/Handler/inlinedDispatcher.hpp*/
    HANDLER_MAIN_DISPATCHER
};

} // namespace DrogonRatServer

/*Defined in DrogonRatServer/Handler/TelegramBotApi.hpp*/
#undef TELEGRAM_BOT_API_HANDLER_METHOD_SIGNATURE_PARAMS
#undef getUpdate_FUNCTION_SIGNATURE

#undef TELEGRAM_BOT_API_MACRO
#undef TELEGRAM_BOT_API_METHOD_ROUTING_MAP
#undef SHELL_INLINED_MACRO
#undef HANDLER_MAIN_DISPATCHER
