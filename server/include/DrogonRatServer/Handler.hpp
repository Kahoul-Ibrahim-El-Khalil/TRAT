/*include/DrogonRatServer/Handler.hpp*/
#pragma once

#include <array>
#include <drogon/drogon.h>
#include <functional>
#include <string>

namespace DrogonRatServer {

class Handler {
public:
  Handler() = default;

  using Method = void (Handler::*)(
      const drogon::HttpRequestPtr &,
      std::function<void(const drogon::HttpResponsePtr &)> &&);

  struct Route {
    std::string path;
    Method method;
  };

  void
  echo(const drogon::HttpRequestPtr &p_Request,
       std::function<void(const drogon::HttpResponsePtr &)> &&arg_Callback);
  void
  download(const drogon::HttpRequestPtr &p_Request,
           std::function<void(const drogon::HttpResponsePtr &)> &&arg_Callback);

  void
  notFound(const drogon::HttpRequestPtr &,
           std::function<void(const drogon::HttpResponsePtr &)> &&arg_Callback);

  void registerRoutes(drogon::HttpAppFramework &Drogon_Singelton);

private:
  const std::array<Route, 2> routes{
      {{"/echo", &Handler::echo}, {"/download", &Handler::download}}};
};

} // namespace DrogonRatServer
