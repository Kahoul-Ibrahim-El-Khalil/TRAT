#include "DrogonRatServer/Handler.hpp"
#include "DrogonRatServer/debug.hpp"
#include <filesystem>
#include <fstream>

namespace DrogonRatServer {

/* Get a reference of the drogon app singleton from the scope of the server,
   which in return gets it from the scope of main.
   Register the routes from the map and call the callback that is the value.
*/
Handler::Handler(drogon::HttpAppFramework &Drogon_App)
    : drogon_app(Drogon_App) {

  // Register routes from static map
  for (const auto &route : routes) {
    this->drogon_app.registerHandler(
        route.path,
        [this, route](const drogon::HttpRequestPtr &p_Request,
                      DrogonHandlerCallback &&cb) {
          (this->*route.method)(p_Request, std::move(cb));
        },
        {drogon::Get, drogon::Post});
  }

  // Upload handler
  this->drogon_app.registerHandler(
      "/upload/{filename}", // filename provided in URL
      [](const drogon::HttpRequestPtr &req,
         std::function<void(const drogon::HttpResponsePtr &)> &&callback,
         const std::string &filename) {
        std::filesystem::create_directories("upload");

        // Save raw body to upload/<filename>
        std::ofstream out("upload/" + filename, std::ios::binary);
        out << req->getBody();
        out.close();

        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setBody("Saved as upload/" + filename);
        callback(resp);
      },
      {drogon::Post, drogon::Put});

  // Echo handler
  this->drogon_app.registerHandler(
      "/echo/{message}", // message provided in URL
      [](const drogon::HttpRequestPtr &req,
         std::function<void(const drogon::HttpResponsePtr &)> &&callback,
         const std::string &message) {
        INFO_LOG("Recieving as message {}", message);
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setBody(message);
        callback(resp);
      },
      {drogon::Get, drogon::Post});
}

} // namespace DrogonRatServer
