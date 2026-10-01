#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"

#include <atomic>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace quill;

struct CapturedMetric
{
  MetricMetadata const* metric_metadata{nullptr};
  double value{0.0};
  std::string thread_id;
  std::string process_id;
  std::string logger_name;
  uint64_t timestamp{0};
};

struct MetricCapturingSink final : public quill::Sink
{
  void write_log(quill::MacroMetadata const*, uint64_t, std::string_view, std::string_view,
                 std::string const&, std::string_view, quill::LogLevel, std::string_view,
                 std::string_view, std::vector<std::pair<std::string, std::string>> const*,
                 std::string_view, std::string_view) override
  {
    log_writes.fetch_add(1, std::memory_order_relaxed);
  }

  void write_metric(quill::MetricMetadata const* metric_metadata, uint64_t log_timestamp,
                    std::string_view thread_id, std::string_view, std::string const& process_id,
                    std::string_view logger_name, double value) override
  {
    std::lock_guard<std::mutex> const lock{mutex};
    metrics.push_back(CapturedMetric{metric_metadata, value, std::string{thread_id}, process_id,
                                     std::string{logger_name}, log_timestamp});
  }

  void flush_sink() noexcept override {}

  std::mutex mutex;
  std::vector<CapturedMetric> metrics;
  std::atomic<size_t> log_writes{0};
};

struct ThrowingFanoutSink final : public quill::Sink
{
  void write_log(quill::MacroMetadata const*, uint64_t, std::string_view, std::string_view,
                 std::string const&, std::string_view, quill::LogLevel, std::string_view,
                 std::string_view, std::vector<std::pair<std::string, std::string>> const*,
                 std::string_view, std::string_view) override
  {
#if !defined(QUILL_NO_EXCEPTIONS)
    throw std::runtime_error{"simulated log sink failure"};
#endif
  }

  void write_metric(quill::MetricMetadata const*, uint64_t, std::string_view, std::string_view,
                    std::string const&, std::string_view, double) override
  {
#if !defined(QUILL_NO_EXCEPTIONS)
    throw std::runtime_error{"simulated metric sink failure"};
#endif
  }

  void flush_sink() noexcept override {}
};

TEST_CASE("metric_sink")
{
  static std::string const throwing_sink_name = "metric_sink_test_throwing_sink";
  static std::string const sink_name = "metric_sink_test_sink";
  static std::string const logger_name = "metric_sink_test_logger";
  static std::string const metric_key = "metric_sink_test_requests_total_01";

  std::atomic<size_t> error_notifications{0};
  BackendOptions backend_options;
  backend_options.error_notifier = [&error_notifications](std::string const&)
  { error_notifications.fetch_add(1, std::memory_order_relaxed); };

  auto throwing_sink = Frontend::create_or_get_sink<ThrowingFanoutSink>(throwing_sink_name);
  auto metric_sink = Frontend::create_or_get_sink<MetricCapturingSink>(sink_name);
  Logger* logger = Frontend::create_or_get_logger(
    logger_name, std::initializer_list<std::shared_ptr<Sink>>{throwing_sink, metric_sink});

  MetricMetadata const* metric_metadata =
    Frontend::create_metric(metric_key, "requests_total", {{"method", "POST"}, {"status", "200"}});

  LOG_INFO(logger, "sink fan-out remains isolated");
  METRIC(logger, metric_metadata, 12.5);
  logger->publish_metric(metric_metadata, 3.25);

  // Change and destroy the caller's strings before the backend can read them.
  {
    std::string name{"metric_sink_test_dynamic"};
    std::vector<MetricLabel> labels{{"status", "200"}, {"method", std::string(64, 'x')}};
    REQUIRE(logger->publish_dynamic_metric(name, labels, 1.0));
    name.assign("changed");
    labels[0].value = "500";
    DYNAMIC_METRIC(logger, "metric_sink_test_dynamic", labels, 2.0);
    std::swap(labels[0], labels[1]);
    labels[1].value = "200";
    QUILL_DYNAMIC_METRIC(logger, "metric_sink_test_dynamic",
                         {{"method", labels[0].value}, {"status", labels[1].value}}, 3.0);

    // Smaller records reuse storage without including stale labels from earlier samples.
    DYNAMIC_METRIC(logger, "metric_sink_test_dynamic", {}, 4.0);
    DYNAMIC_METRIC(logger, "metric_sink_test_dynamic", {{"method", labels[0].value}}, 5.0);
    DYNAMIC_METRIC(logger, "metric_sink_test_dynamic", labels, 6.0);

    // Length-prefixed identities distinguish separators and embedded nulls.
    DYNAMIC_METRIC(logger, "metric_sink_test_identity", {{"a", "b:c"}, {"d", "e"}}, 7.0);
    DYNAMIC_METRIC(logger, "metric_sink_test_identity", {{"a:b", "c"}, {"d", "e"}}, 8.0);
    DYNAMIC_METRIC(logger, "metric_sink_test_identity", {{"a", std::string{"b\0c", 3}}, {"d", "e"}}, 9.0);
  }

  Backend::start(backend_options);
  logger->flush_log();

  Backend::stop();
  Frontend::remove_logger(logger);

  auto* sink_ptr = static_cast<MetricCapturingSink*>(metric_sink.get());
  REQUIRE_EQ(sink_ptr->log_writes.load(std::memory_order_relaxed), 1);
#if !defined(QUILL_NO_EXCEPTIONS)
  REQUIRE_EQ(error_notifications.load(std::memory_order_relaxed), 2);
#else
  REQUIRE_EQ(error_notifications.load(std::memory_order_relaxed), 0);
#endif

  std::lock_guard<std::mutex> const lock{sink_ptr->mutex};
  REQUIRE_EQ(sink_ptr->metrics.size(), 11);

  REQUIRE_EQ(sink_ptr->metrics[0].metric_metadata, metric_metadata);
  REQUIRE_EQ(sink_ptr->metrics[0].metric_metadata->metric_key(), metric_key);
  REQUIRE_EQ(sink_ptr->metrics[0].metric_metadata->metric_name(), "requests_total");
  REQUIRE_EQ(sink_ptr->metrics[0].metric_metadata->labels().size(), 2);
  REQUIRE_EQ(sink_ptr->metrics[0].metric_metadata->labels()[0].key, "method");
  REQUIRE_EQ(sink_ptr->metrics[0].metric_metadata->labels()[0].value, "POST");
  REQUIRE_EQ(sink_ptr->metrics[0].metric_metadata->labels()[1].key, "status");
  REQUIRE_EQ(sink_ptr->metrics[0].metric_metadata->labels()[1].value, "200");
  REQUIRE_EQ(sink_ptr->metrics[0].logger_name, logger_name);
  REQUIRE_FALSE(sink_ptr->metrics[0].thread_id.empty());
  REQUIRE_FALSE(sink_ptr->metrics[0].process_id.empty());
  REQUIRE_GT(sink_ptr->metrics[0].timestamp, 0);
  REQUIRE_EQ(sink_ptr->metrics[0].value, doctest::Approx{12.5});

  REQUIRE_EQ(sink_ptr->metrics[1].metric_metadata, metric_metadata);
  REQUIRE_EQ(sink_ptr->metrics[1].logger_name, logger_name);
  REQUIRE_FALSE(sink_ptr->metrics[1].thread_id.empty());
  REQUIRE_FALSE(sink_ptr->metrics[1].process_id.empty());
  REQUIRE_GT(sink_ptr->metrics[1].timestamp, 0);
  REQUIRE_EQ(sink_ptr->metrics[1].value, doctest::Approx{3.25});

  auto const* dynamic_metric = sink_ptr->metrics[2].metric_metadata;
  REQUIRE_EQ(dynamic_metric->metric_name(), "metric_sink_test_dynamic");
  REQUIRE_EQ(dynamic_metric->labels().size(), 2);
  REQUIRE_EQ(dynamic_metric->labels()[0].key, "method");
  REQUIRE_EQ(dynamic_metric->labels()[0].value, std::string(64, 'x'));
  REQUIRE_EQ(dynamic_metric->labels()[1].value, "200");
  REQUIRE_EQ(sink_ptr->metrics[2].value, doctest::Approx{1.0});
  REQUIRE_NE(sink_ptr->metrics[3].metric_metadata, dynamic_metric);
  REQUIRE_EQ(sink_ptr->metrics[3].metric_metadata->labels()[1].value, "500");
  REQUIRE_EQ(sink_ptr->metrics[3].value, doctest::Approx{2.0});
  REQUIRE_EQ(sink_ptr->metrics[4].metric_metadata, dynamic_metric);
  REQUIRE_EQ(sink_ptr->metrics[4].value, doctest::Approx{3.0});

  REQUIRE(sink_ptr->metrics[5].metric_metadata->labels().empty());
  REQUIRE_EQ(sink_ptr->metrics[5].value, doctest::Approx{4.0});
  REQUIRE_EQ(sink_ptr->metrics[6].metric_metadata->labels().size(), 1);
  REQUIRE_EQ(sink_ptr->metrics[6].metric_metadata->labels()[0].key, "method");
  REQUIRE_EQ(sink_ptr->metrics[6].value, doctest::Approx{5.0});
  REQUIRE_EQ(sink_ptr->metrics[7].metric_metadata, dynamic_metric);
  REQUIRE_EQ(sink_ptr->metrics[7].value, doctest::Approx{6.0});

  auto const* separator_metric = sink_ptr->metrics[8].metric_metadata;
  REQUIRE_EQ(separator_metric->labels()[0].value, "b:c");
  REQUIRE_NE(sink_ptr->metrics[9].metric_metadata, separator_metric);
  REQUIRE_NE(sink_ptr->metrics[9].metric_metadata->metric_key(), separator_metric->metric_key());
  REQUIRE_NE(sink_ptr->metrics[10].metric_metadata, separator_metric);
  REQUIRE_EQ(sink_ptr->metrics[10].metric_metadata->labels()[0].value, std::string("b\0c", 3));
}
