#include "httpd.hpp"

HttpServer::~HttpServer() {
    stop();
}

bool HttpServer::start(uint16_t port) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    config.max_uri_handlers = static_cast<uint16_t>(routes_.size() + 4);

    if (httpd_start(&server_, &config) != ESP_OK) return false;

    for (const auto &route : routes_) {
        registerRoute(*route);
    }
    return true;
}

void HttpServer::stop() {
    if (server_) {
        httpd_stop(server_);
        server_ = nullptr;
    }
}

void HttpServer::addGet(const char *uri, GetHandler handler) {
    routes_.push_back(std::make_unique<Route>(Route{uri, std::move(handler)}));
    if (server_) registerRoute(*routes_.back());
}

void HttpServer::registerRoute(const Route &route) {
    httpd_uri_t uri_cfg = {
        .uri     = route.uri.c_str(),
        .method  = HTTP_GET,
        .handler = staticHandler,
        .user_ctx = const_cast<Route *>(&route),
    };
    httpd_register_uri_handler(server_, &uri_cfg);
}

esp_err_t HttpServer::staticHandler(httpd_req_t *req) {
    auto *route = static_cast<Route *>(req->user_ctx);
    std::string body = route->handler();
    httpd_resp_set_type(req, "text/plain; version=0.0.4; charset=utf-8");
    httpd_resp_send(req, body.c_str(), static_cast<ssize_t>(body.size()));
    return ESP_OK;
}
