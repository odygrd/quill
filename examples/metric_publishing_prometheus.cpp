#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"
#include "quill/sinks/metrics/PrometheusSink.h"

#include <iostream>
#include <string>
#include <utility>

/**
 * This example shows regular logging to a ConsoleSink and metric publishing to a real
 * prometheus-cpp registry through `quill::PrometheusSink`.
 *
 * Quill keeps metric metadata generic. The Prometheus-specific registration happens on the sink:
 * the sink decides whether a metric is exported as a counter, gauge, histogram, or summary.
 * `PrometheusSink` infers the Prometheus type from how the caller registered the metric
 * (register_counter / register_gauge / register_histogram / register_summary).
 */
int main()
{
  // Start the backend worker before creating loggers and publishing metrics.
  quill::Backend::start();

  // Regular application logs go to the console.
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

  // Regular logs use the normal logging API.
  LOG_INFO(logger, "{} Set up metrics example {}", "===", "===");

  // Register Quill metric metadata once and keep the returned pointers for the hot path.
  quill::MetricMetadata const* requests_total_metric = quill::Frontend::create_metric(
    "requests_total", "requests_total", {{"method", "POST"}, {"status", "500"}});

  quill::MetricMetadata const* in_flight_requests_metric =
    quill::Frontend::create_metric("in_flight_requests", "in_flight_requests");

  quill::MetricMetadata const* request_latency_metric = quill::Frontend::create_metric(
    "request_latency_seconds", "request_latency_seconds", {{"method", "POST"}});

  // The sink owns the prometheus-cpp registry and can optionally start an HTTP exposer.
  quill::PrometheusSink::ExposerConfiguration exposer_config;
  exposer_config.bind_address = "127.0.0.1:45165";
  exposer_config.num_threads = 1;

  // Create Prometheus Sink
  auto prom_sink = std::static_pointer_cast<quill::PrometheusSink>(
    quill::Frontend::create_or_get_sink<quill::PrometheusSink>("prometheus_sink", exposer_config));

  // Prometheus-specific registration lives on the sink.
  prom_sink->register_counter(requests_total_metric, "Total number of handled requests");

  prom_sink->register_gauge(in_flight_requests_metric, "Current in-flight request count",
                            quill::PrometheusSink::GaugeUpdateMode::Add);

  prom_sink->register_histogram(request_latency_metric, "Request latency in seconds",
                                {0.005, 0.01, 0.05, 0.1, 0.5, 1.0, 5.0});

  LOG_INFO(logger, "scrape_endpoint: {}", prom_sink->exposer_scrape_endpoint());

  // Create metrics logger to sink
  quill::Logger* metrics_logger = quill::Frontend::create_or_get_logger("metrics", std::move(prom_sink));

  int32_t requests_total{0};
  while (true)
  {
    std::this_thread::sleep_for(std::chrono::seconds{20});

    // update metrics - macros for consistency
    METRIC(metrics_logger, requests_total_metric, ++requests_total);
    METRIC(metrics_logger, in_flight_requests_metric, 1);

    // or the as function
    metrics_logger->publish_metric(request_latency_metric, 3.0);
    metrics_logger->publish_metric(requests_total_metric, 1.0);
  }
}
