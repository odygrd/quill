#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/Logger.h"
#include "quill/sinks/metrics/PrometheusSink.h"

#include <prometheus/histogram.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <string>
#include <utility>

namespace
{
/** @return the number of samples of the histogram series `family_name` with the given label */
uint64_t histogram_sample_count(quill::PrometheusSink& prometheus_sink, std::string const& family_name,
                                std::string const& label_name, std::string const& label_value)
{
  for (auto const& family : prometheus_sink.registry().Collect())
  {
    if (family.name != family_name)
    {
      continue;
    }

    for (auto const& metric : family.metric)
    {
      for (auto const& label : metric.label)
      {
        if ((label.name == label_name) && (label.value == label_value))
        {
          return metric.histogram.sample_count;
        }
      }
    }
  }

  return 0;
}
} // namespace

int main()
{
  quill::Backend::start();

  quill::MetricMetadata const* requests_total =
    quill::Frontend::create_metric("requests_total", "requests_total", {{"method", "GET"}});

  // No exposer: the registry is only updated in process
  auto prometheus_sink =
    quill::Frontend::create_or_get_sink<quill::PrometheusSink>("prometheus_sink");
  prometheus_sink->register_counter(requests_total, "Total number of handled requests");

  // A rejected registration must not stop the samples of its labels once the family is registered
  quill::MetricMetadata const* route_a_latency = quill::Frontend::create_metric(
    "request_latency_seconds_a", "request_latency_seconds", {{"route", "a"}});

  try
  {
    prometheus_sink->register_histogram(route_a_latency, "Request latency", {0.5, 0.1});
  }
  catch (std::exception const&)
  {
    // prometheus-cpp rejects unsorted bucket boundaries
  }

  prometheus_sink->register_histogram_family("request_latency_seconds", "Request latency", {0.1, 0.5});

  // A rejected registration must not keep the help text of the family it created, so a retry that
  // also corrects the help text succeeds
  quill::MetricMetadata const* queue_latency = quill::Frontend::create_metric(
    "queue_latency_seconds", "queue_latency_seconds", {{"queue", "orders"}});

  try
  {
    prometheus_sink->register_histogram(queue_latency, "Queue latncy", {0.5, 0.1});
  }
  catch (std::exception const&)
  {
    // prometheus-cpp rejects unsorted bucket boundaries
  }

  prometheus_sink->register_histogram(queue_latency, "Queue latency", {0.1, 0.5});

  // A rejected registration must leave a family the application registered directly untouched
  prometheus::BuildHistogram()
    .Name("app_latency_seconds")
    .Help("Application latency")
    .Register(prometheus_sink->registry())
    .Add({{"source", "app"}}, prometheus::Histogram::BucketBoundaries{0.1, 0.5})
    .Observe(0.2);

  quill::MetricMetadata const* app_latency = quill::Frontend::create_metric(
    "app_latency_seconds", "app_latency_seconds", {{"source", "quill"}});

  try
  {
    prometheus_sink->register_histogram(app_latency, "Application latency", {0.5, 0.1});
  }
  catch (std::exception const&)
  {
    // prometheus-cpp rejects unsorted bucket boundaries
  }

  // The series of the sink joins the family that the application registered. Unregistering it
  // must leave that family, which still has the series of the application, in the registry
  prometheus_sink->register_histogram(app_latency, "Application latency", {0.1, 0.5});
  prometheus_sink->unregister_metric(app_latency);

  // The application registers a family and keeps it, without a series yet. The series of the
  // sink joins it and is unregistered, which leaves the family without series. It is still the
  // family of the application, which adds its series to it afterwards
  auto& app_owned_family = prometheus::BuildHistogram()
                             .Name("app_owned_seconds")
                             .Help("Application owned")
                             .Register(prometheus_sink->registry());

  quill::MetricMetadata const* app_owned =
    quill::Frontend::create_metric("app_owned_seconds", "app_owned_seconds", {{"source", "quill"}});

  prometheus_sink->register_histogram(app_owned, "Application owned", {0.1, 0.5});
  prometheus_sink->unregister_metric(app_owned);

  app_owned_family.Add({{"source", "app"}}, prometheus::Histogram::BucketBoundaries{0.1, 0.5}).Observe(0.2);

  // A family registration with invalid buckets must be rejected, because a later registration of
  // the family does not replace the buckets its runtime series are created with
  try
  {
    prometheus_sink->register_histogram_family("batch_size", "Batch size", {10.0, 1.0});
  }
  catch (std::exception const&)
  {
    // prometheus-cpp rejects unsorted bucket boundaries
  }

  prometheus_sink->register_histogram_family("batch_size", "Batch size", {1.0, 10.0});

  // A summary window that prometheus-cpp would divide by zero for, or rotate without end, is
  // rejected before anything is registered, so the corrected registration succeeds
  quill::MetricMetadata const* response_size =
    quill::Frontend::create_metric("response_size_bytes", "response_size_bytes", {{"route", "a"}});

  struct SummaryWindow
  {
    std::chrono::milliseconds max_age;
    int age_buckets;
  };

  // The last two cannot be converted to the duration that prometheus-cpp keeps the window in
  SummaryWindow const invalid_summary_windows[] = {{std::chrono::seconds{60}, 0},
                                                   {std::chrono::seconds{60}, -1},
                                                   {std::chrono::milliseconds{0}, 5},
                                                   {(std::chrono::milliseconds::max)(), 5},
                                                   {(std::chrono::milliseconds::min)(), 5}};

  size_t accepted_invalid_summaries{0};
  for (SummaryWindow const& window : invalid_summary_windows)
  {
    try
    {
      prometheus_sink->register_summary(response_size, "Response size", {{0.5, 0.05}},
                                        window.max_age, window.age_buckets);
      ++accepted_invalid_summaries;
    }
    catch (std::exception const&)
    {
    }

    try
    {
      prometheus_sink->register_summary_family("request_size_bytes", "Request size", {{0.5, 0.05}},
                                               window.max_age, window.age_buckets);
      ++accepted_invalid_summaries;
    }
    catch (std::exception const&)
    {
    }
  }

  prometheus_sink->register_summary(response_size, "Response size", {{0.5, 0.05}});
  prometheus_sink->register_summary_family("request_size_bytes", "Request size", {{0.5, 0.05}});

  quill::Logger* logger = quill::Frontend::create_or_get_logger("prometheus_logger", prometheus_sink);

  logger->publish_metric(requests_total, 1.0);
  logger->publish_dynamic_metric("dynamic_requests_total", {{"method", "POST"}}, 1.0);
  logger->publish_metric(route_a_latency, 0.2);
  logger->publish_metric(queue_latency, 0.2);
  logger->publish_dynamic_metric("batch_size", {{"stage", "decode"}}, 5.0);
  logger->publish_metric(response_size, 512.0);
  logger->publish_dynamic_metric("request_size_bytes", {{"route", "a"}}, 256.0);

  logger->flush_log();

  struct ExpectedSample
  {
    char const* family_name;
    char const* label_name;
    char const* label_value;
  };

  ExpectedSample const expected_samples[] = {{"request_latency_seconds", "route", "a"},
                                             {"queue_latency_seconds", "queue", "orders"},
                                             {"app_latency_seconds", "source", "app"},
                                             {"app_owned_seconds", "source", "app"},
                                             {"batch_size", "stage", "decode"}};

  size_t missing_samples{0};
  for (ExpectedSample const& expected : expected_samples)
  {
    uint64_t const sample_count = histogram_sample_count(*prometheus_sink, expected.family_name,
                                                         expected.label_name, expected.label_value);

    if (sample_count != 1)
    {
      std::fprintf(stderr, "expected 1 sample for %s{%s=\"%s\"}, got %llu\n", expected.family_name,
                   expected.label_name, expected.label_value, static_cast<unsigned long long>(sample_count));
      ++missing_samples;
    }
  }

  if (accepted_invalid_summaries != 0)
  {
    std::fprintf(stderr, "%zu summaries with an invalid window were registered\n", accepted_invalid_summaries);
  }

  quill::Frontend::remove_logger(logger);
  quill::Backend::stop();

  return ((missing_samples == 0) && (accepted_invalid_summaries == 0)) ? 0 : 1;
}
