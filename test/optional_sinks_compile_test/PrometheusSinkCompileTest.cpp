#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/Logger.h"
#include "quill/sinks/metrics/PrometheusSink.h"

#include <utility>

int main()
{
  quill::Backend::start();

  quill::MetricMetadata const* requests_total =
    quill::Frontend::create_metric("requests_total", "requests_total", {{"method", "GET"}});

  // No exposer: the registry is only updated in process
  auto prometheus_sink =
    quill::Frontend::create_or_get_sink<quill::PrometheusSink>("prometheus_sink");
  prometheus_sink->register_counter(requests_total, "Total number of handled requests");

  quill::Logger* logger = quill::Frontend::create_or_get_logger("prometheus_logger", prometheus_sink);

  logger->publish_metric(requests_total, 1.0);
  logger->publish_dynamic_metric("dynamic_requests_total", {{"method", "POST"}}, 1.0);

  logger->flush_log();
  quill::Frontend::remove_logger(logger);
  quill::Backend::stop();
}
