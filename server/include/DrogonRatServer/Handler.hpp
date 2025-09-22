/*include/DrogonRatServer/Handler.hpp*/
#pragma once
#include <array>
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpResponse.h>
#include <drogon/drogon.h>
#include <functional>
#include <string>

#define HANDLER_ROUTE_MAP                                                      \
  {                                                                            \
    { "/hello", &Handler::hello }                                              \
  }

namespace DrogonRatServer {
using DrogonHandlerCallback =
    std::function<void(const drogon::HttpResponsePtr &)>;

class Handler {
public:
  using Method = void (Handler::*)(const drogon::HttpRequestPtr &,
                                   DrogonHandlerCallback &&);
  struct Route {
    std::string path;
    Method method;
  };

private:
  const std::array<Route, 1> routes = HANDLER_ROUTE_MAP;

public:
  drogon::HttpAppFramework &drogon_app;

  /*Default constructors must be deleted*/
  Handler() = delete;
  Handler(const Handler &Other_Handler) = delete;
  Handler(Handler &&Other_Handler) = delete;

  Handler(drogon::HttpAppFramework &Drogon_App);

  void hello(const drogon::HttpRequestPtr &p_Request,
             DrogonHandlerCallback &&arg_Callback);

  void notFound(const drogon::HttpRequestPtr &,
                DrogonHandlerCallback &&arg_Callback);
};

} // namespace DrogonRatServer
#undef HANDLER_ROUTE_MAP
