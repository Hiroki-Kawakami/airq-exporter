#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class PrometheusExporter {
public:
    enum class MetricType { Gauge, Counter };

    class Metric {
    public:
        void set(double value) { value_.store(value, std::memory_order_relaxed); }
        void increment(double by = 1.0);
        double get() const { return value_.load(std::memory_order_relaxed); }

    private:
        std::atomic<double> value_{0.0};
    };

    Metric *addGauge(const char *name, const char *help,
                     std::vector<std::pair<const char *, const char *>> labels = {});
    Metric *addCounter(const char *name, const char *help,
                       std::vector<std::pair<const char *, const char *>> labels = {});

    template <typename HttpServer>
    bool start(HttpServer &server, uint16_t port = 9090) {
        server.addGet("/metrics", [this]() { return buildResponse(); });
        return server.start(port);
    }

private:
    struct MetricEntry {
        std::string name;
        std::string help;
        MetricType type;
        std::vector<std::pair<std::string, std::string>> labels;
        Metric metric;
    };

    std::vector<std::unique_ptr<MetricEntry>> metrics_;

    Metric *addMetric(MetricType type, const char *name, const char *help,
                      std::vector<std::pair<const char *, const char *>> labels);
    std::string buildResponse() const;
};
