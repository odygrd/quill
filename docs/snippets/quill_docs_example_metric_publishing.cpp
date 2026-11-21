#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/Sink.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class MyMetricSink final : public quill::Sink
{
public:
  void write_log(quill::LogRecord const& /* record */) override {}

  void flush_sink(quill::SinkFlushReason) noexcept override {}

  // Called on the backend worker thread for every published sample.
  void write_metric(quill::MetricRecord const& record) override
  {
    // Export however you like: Prometheus, StatsD, OpenTelemetry, a socket...
    std::cout << record.metric_metadata->metric_name() << " = " << record.value << '\n';
  }
};

int main()
{
  quill::Backend::start();

  auto sink = quill::Frontend::create_or_get_sink<MyMetricSink>("my_metric_sink");
  quill::Logger* logger = quill::Frontend::create_or_get_logger("metrics", std::move(sink));

  // Register metric metadata once. The returned pointer is stable for the program lifetime.
  quill::MetricMetadata const* requests_total = quill::Frontend::create_metric(
    "requests_total_post_200", "requests_total", {{"method", "POST"}, {"status", "200"}});

  // Publish samples from the hot path.
  logger->publish_metric(requests_total, 1.0);

  // Optional convenience outside latency-sensitive hot paths: copies names and labels.
  // This custom sink receives samples directly; family registration is specific to PrometheusSink.
  DYNAMIC_METRIC(logger, "requests_total", {{"method", "GET"}, {"status", "200"}}, 1.0);
}
