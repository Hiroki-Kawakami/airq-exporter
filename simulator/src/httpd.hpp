#pragma once
#include <httplib.h>
#include <functional>
#include <string>
#include <thread>

class HttpServer {
public:
    using GetHandler = std::function<std::string()>;

    HttpServer() = default;
    ~HttpServer() { stop(); }

    HttpServer(const HttpServer &) = delete;
    HttpServer &operator=(const HttpServer &) = delete;

    void addGet(const char *uri, GetHandler handler) {
        server_.Get(uri, [h = std::move(handler)](const httplib::Request &, httplib::Response &res) {
            res.set_content(h(), "text/plain; version=0.0.4; charset=utf-8");
        });
    }

    bool start(uint16_t port) {
        thread_ = std::thread([this, port]() {
            server_.listen("0.0.0.0", static_cast<int>(port));
        });
        return true;
    }

    void stop() {
        server_.stop();
        if (thread_.joinable()) thread_.join();
    }

private:
    httplib::Server server_;
    std::thread thread_;
};
