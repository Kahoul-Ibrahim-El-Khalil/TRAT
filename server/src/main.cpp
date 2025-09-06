
#include "rat/server.hpp"

#include <array>
#include <json/json.h>
#include <string>

int main() {
	// Define one endpoint at "/hello"
	std::array<::rat::server::EndpointHandlerLambdaPair, 1> endpoints = {
	    {{"/hello", [](const drogon::HttpRequestPtr &) -> Json::Value {
		      Json::Value json;
		      json["msg"] = "hello world!";
		      return json;
	      }}}};

	// Run server on localhost:8080, without TLS for simplicity
	::rat::server::DrogonServer<1> server("192.168.1.10", 8080, "", "",
	                                      endpoints);

	server.run();

	return 0;
}
