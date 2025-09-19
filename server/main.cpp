#include <drogon/drogon.h>

// A simple handler for /echo
static void echoHandler(const drogon::HttpRequestPtr &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k200OK);
    resp->setContentTypeCode(drogon::CT_TEXT_PLAIN);
    resp->setBody("hello world");
    callback(resp);
}

// Static mapping of paths → handlers
static const std::unordered_map<std::string, HandlerFunc> routeTable = {
    {"/echo", echoHandler},   // notice: echoHandler is convertible to HandlerFunc
};

// Dispatcher
static void dispatchHandler(const drogon::HttpRequestPtr &req,
                            std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    auto it = routeTable.find(req->path());
    if (it != routeTable.end()) {
        it->second(req, std::move(callback));
    } else {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k404NotFound);
        resp->setBody("404 Not Found");
        callback(resp);
    }
}

int main() {
    // Register dispatcher (lambda wraps function, avoids raw function pointer)
    drogon::app().registerHandler(
        "/echo",
        [](const drogon::HttpRequestPtr &req,
           std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
            dispatchHandler(req, std::move(cb));
        },
        {drogon::Get, drogon::Post});

    drogon::app().addListener("0.0.0.0", 8080).run();
}
