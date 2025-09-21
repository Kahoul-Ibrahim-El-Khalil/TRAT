#include "DrogonRatServer/Handler.hpp"

namespace DrogonRatServer {

void Handler::echo(
    const drogon::HttpRequestPtr &p_Request,
    std::function<void(const drogon::HttpResponsePtr &)> &&arg_Callback) {
  auto resp = drogon::HttpResponse::newHttpResponse();
  resp->setStatusCode(drogon::k200OK);
  resp->setContentTypeCode(drogon::CT_TEXT_PLAIN);
  resp->setBody("hello world");
  arg_Callback(resp);
}

void Handler::notFound(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&arg_Callback) {
  auto resp = drogon::HttpResponse::newHttpResponse();
  resp->setStatusCode(drogon::k404NotFound);
  resp->setBody("404 Not Found");
  arg_Callback(resp);
}

void Handler::registerRoutes(drogon::HttpAppFramework &Drogon_Singelton) {
  for (const auto &route :
       routes) { // capture route by const reference for safety
    Drogon_Singelton.registerHandler(
        std::string(route.path), // convert string_view to string
        [this,
         route](const drogon::HttpRequestPtr &p_Request,
                std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
          (this->*route.method)(p_Request, std::move(cb));
        },
        {drogon::Get, drogon::Post});
  }
}

} // namespace DrogonRatServer
