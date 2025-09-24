/*include/DrogonRatServer/Handler.hpp*/
#pragma once
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>
#include <drogon/orm/Mapper.h>
#include <functional>

namespace DrogonRatServer {
using DrogonHandlerCallback =
    std::function<void(const drogon::HttpResponsePtr &)>;

class Handler {
  private:
	std::filesystem::path db_file_path;
	drogon::orm::DbClientPtr p_db_client;

	std::vector<std::string> cached_tokens = {};

  public:
	drogon::HttpAppFramework &drogon_app; // The parent class of this calls it,
	/*Default constructors must be deleted*/
	Handler() = delete;
	Handler(const Handler &Other_Handler) = delete;
	Handler(Handler &&Other_Handler) = delete;

	Handler(drogon::HttpAppFramework &Drogon_App);

	Handler &setDbFilePath(const std::filesystem::path &Db_File_Path);
	Handler &initDbClient(void);

	Handler &registerAll();
	// Implimentation in src/Handler/methods.cpp
  protected:
	void registerUploadHandler();
	void registerEchoHandler();

	void registerTelegramBotApiHandler();
};

} // namespace DrogonRatServer

#undef HANDLER_ROUTE_MAP
