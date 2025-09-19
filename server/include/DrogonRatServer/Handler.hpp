/*include/DrogonRatServer/Handler.hpp*/
#pragma once

#include <drogon/drogon.h>
#include <string>
#include <array>
#include <functional>

namespace DrogonRatServer {

class Handler {
  public:
    Handler() = default;

    using Method = void (Handler::*)(
        const drogon::HttpRequestPtr &,
        std::function<void(const drogon::HttpResponsePtr &)> &&);

    struct Route {
        std::string_view path;
        Method method;
    };

    void echo(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback); 

    void notFound(const drogon::HttpRequestPtr &,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback); 

    void registerRoutes(drogon::HttpAppFramework &app);
  private:
    const std::array<Route, 1> routes{{
        {"/echo", &Handler::echo},
    }};
};

} // namespace DrogonRatServer
