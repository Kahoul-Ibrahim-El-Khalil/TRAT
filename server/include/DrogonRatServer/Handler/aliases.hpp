#pragma once

namespace DrogonRatServer {

using DrogonHandlerCallback = std::function<void(const drogon::HttpResponsePtr &)>;
using HttpResponseCallback = std::function<void(const drogon::HttpResponsePtr &)>;
using HttpResponseCallbackPtr = std::shared_ptr<HttpResponseCallback>;

} // namespace DrogonRatServer
