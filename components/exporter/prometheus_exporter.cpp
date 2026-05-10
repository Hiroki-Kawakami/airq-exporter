#include "prometheus_exporter.hpp"
#include <cstdio>
#include <set>

void PrometheusExporter::Metric::increment(double by) {
    double old = value_.load(std::memory_order_relaxed);
    while (!value_.compare_exchange_weak(old, old + by, std::memory_order_relaxed));
}

PrometheusExporter::Metric *PrometheusExporter::addMetric(
    MetricType type, const char *name, const char *help,
    std::vector<std::pair<const char *, const char *>> labels)
{
    auto entry = std::make_unique<MetricEntry>();
    entry->name = name;
    entry->help = help;
    entry->type = type;
    for (const auto &[k, v] : labels) {
        entry->labels.emplace_back(k, v);
    }
    auto *metric = &entry->metric;
    metrics_.push_back(std::move(entry));
    return metric;
}

PrometheusExporter::Metric *PrometheusExporter::addGauge(
    const char *name, const char *help,
    std::vector<std::pair<const char *, const char *>> labels)
{
    return addMetric(MetricType::Gauge, name, help, std::move(labels));
}

PrometheusExporter::Metric *PrometheusExporter::addCounter(
    const char *name, const char *help,
    std::vector<std::pair<const char *, const char *>> labels)
{
    return addMetric(MetricType::Counter, name, help, std::move(labels));
}

std::string PrometheusExporter::buildResponse() const {
    std::string out;
    out.reserve(512);

    std::set<std::string> seen;
    for (const auto &e : metrics_) {
        if (seen.insert(e->name).second) {
            out += "# HELP ";
            out += e->name;
            out += ' ';
            out += e->help;
            out += '\n';
            out += "# TYPE ";
            out += e->name;
            out += ' ';
            out += (e->type == MetricType::Gauge) ? "gauge" : "counter";
            out += '\n';
        }

        out += e->name;
        if (!e->labels.empty()) {
            out += '{';
            for (size_t i = 0; i < e->labels.size(); ++i) {
                if (i) out += ',';
                out += e->labels[i].first;
                out += "=\"";
                out += e->labels[i].second;
                out += '"';
            }
            out += '}';
        }
        out += ' ';
        char buf[32];
        snprintf(buf, sizeof(buf), "%g", e->metric.get());
        out += buf;
        out += '\n';
    }

    return out;
}
