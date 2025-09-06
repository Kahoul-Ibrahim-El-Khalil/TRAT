#pragma once
#include <array>
#include <drogon/drogon.h>
#include <functional>
#include <string>

namespace rat::server {

using HandlerLambda =
    std::function<Json::Value(const drogon::HttpRequestPtr &)>;

struct EndpointHandlerLambdaPair {
	std::string endpoint_path;
	HandlerLambda handler;
};

template <size_t N> class DrogonServer {
  private:
	std::string ip;
	uint16_t port;
	std::string certificate;
	std::string key;
	std::array<EndpointHandlerLambdaPair, N> endpoint_to_handler_map;

  public:
	DrogonServer(std::string arg_Ip, uint16_t arg_Port,
	             std::string SSL_Certificate, std::string SSL_Key,
	             std::array<EndpointHandlerLambdaPair, N> &arg_Endpoints)
	    : ip(std::move(arg_Ip)), port(arg_Port),
	      certificate(std::move(SSL_Certificate)), key(std::move(SSL_Key)),
	      endpoint_to_handler_map(std::move(arg_Endpoints)) {
	}

	void run() {
			for(const auto &entry : this->endpoint_to_handler_map) {
				drogon::app().registerHandler(
				    entry.endpoint_path,
				    [handler = entry.handler](
				        const drogon::HttpRequestPtr &req,
				        std::function<void(const drogon::HttpResponsePtr &)>
				            &&callback) {
					    auto json = handler(req);
					    auto resp =
					        drogon::HttpResponse::newHttpJsonResponse(json);
					    callback(resp);
				    },
				    {drogon::Get, drogon::Post});
			}

			if(!this->certificate.empty() && !this->key.empty()) {
				drogon::app()
				    .addListener(this->ip, this->port, true, this->certificate,
				                 this->key)
				    .run();
			}
			else {
				drogon::app().addListener(this->ip, this->port).run();
			}
	}
};
} // namespace rat::server
