#include "DrogonRatServer/Handler.hpp"

namespace DrogonRatServer {

void Handler::echo(const drogon::HttpRequestPtr &req,
                   std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k200OK);
    resp->setContentTypeCode(drogon::CT_TEXT_PLAIN);
    resp->setBody("hello world");
    callback(resp);
}

void Handler::notFound(const drogon::HttpRequestPtr &,
                       std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k404NotFound);
    resp->setBody("404 Not Found");
    callback(resp);
}

void Handler::registerRoutes(drogon::HttpAppFramework &app) {
    for (const auto &route : routes) { // capture route by const reference for safety
        app.registerHandler(
            std::string(route.path), // convert string_view to string
            [this, route](const drogon::HttpRequestPtr &req,
                          std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
                (this->*route.method)(req, std::move(cb));
            },
            {drogon::Get, drogon::Post});
    }
}

}//namespace DrogonRatServer 
