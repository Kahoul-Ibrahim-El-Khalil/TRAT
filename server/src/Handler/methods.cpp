#include "DrogonRatServer/Handler.hpp"
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>

namespace DrogonRatServer {

void Handler::hello(const drogon::HttpRequestPtr &p_Request,
                    DrogonHandlerCallback &&arg_Callback) {
  auto resp = drogon::HttpResponse::newHttpResponse();
  resp->setStatusCode(drogon::k200OK);
  resp->setContentTypeCode(drogon::CT_TEXT_PLAIN);
  resp->setBody("Hi");
  arg_Callback(resp);
}

void Handler::notFound(const drogon::HttpRequestPtr &,
                       DrogonHandlerCallback &&arg_Callback) {
  auto resp = drogon::HttpResponse::newHttpResponse();
  resp->setStatusCode(drogon::k404NotFound);
  resp->setBody("404 Not Found");
  arg_Callback(resp);
}

} // namespace DrogonRatServer
