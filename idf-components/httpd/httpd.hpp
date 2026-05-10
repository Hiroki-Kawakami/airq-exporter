#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "esp_http_server.h"

class HttpServer {
public:
    using GetHandler = std::function<std::string()>;

    HttpServer() = default;
    ~HttpServer();

    HttpServer(const HttpServer &) = delete;
    HttpServer &operator=(const HttpServer &) = delete;

    bool start(uint16_t port);
    void stop();
    void addGet(const char *uri, GetHandler handler);

private:
    struct Route {
        std::string uri;
        GetHandler handler;
    };

    httpd_handle_t server_ = nullptr;
    std::vector<std::unique_ptr<Route>> routes_;

    void registerRoute(const Route &route);
    static esp_err_t staticHandler(httpd_req_t *req);
};
