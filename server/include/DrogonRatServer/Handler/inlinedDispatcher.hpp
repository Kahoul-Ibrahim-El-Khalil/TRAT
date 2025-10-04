#pragma once

#define HANDLER_MAIN_DISPATCHER                                                                    \
  protected:                                                                                       \
    inline void _dispatchHandlerAccordingToMethod(                                                 \
        const std::string &Method_Literal,                                                         \
        const drogon::HttpRequestPtr &arg_Req,                                                     \
        Bot *p_Bot,                                                                                \
        const std::string &arg_Token,                                                              \
        drogon::orm::DbClientPtr &p_db_client,                                                     \
        std::function<void(const drogon::HttpResponsePtr &)> &&arg_Callback) {                     \
        for(const auto &[method_name, handler] : method_handler_map) {                             \
            if(Method_Literal == method_name) {                                                    \
                handler(arg_Req, p_Bot, arg_Token, p_db_client, std::move(arg_Callback));          \
                return;                                                                            \
            }                                                                                      \
        }                                                                                          \
                                                                                                   \
        auto resp = drogon::HttpResponse::newHttpResponse();                                       \
        resp->setStatusCode(drogon::k404NotFound);                                                 \
        const std::string message = fmt::format("Method not found {}", Method_Literal);            \
        DEBUG_LOG("{}", message);                                                                  \
        resp->setBody(message);                                                                    \
        arg_Callback(resp);                                                                        \
    }
