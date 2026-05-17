#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"

#include <atomic>
#include <mutex>
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

TEST_CASE("metric_sink")
{
  static std::string const sink_name = "metric_sink_test_sink";
  static std::string const logger_name = "metric_sink_test_logger";
  static std::string const metric_key = "metric_sink_test_requests_total_01";

  Backend::start();

  auto metric_sink = Frontend::create_or_get_sink<MetricCapturingSink>(sink_name);
  Logger* logger = Frontend::create_or_get_logger(logger_name, metric_sink);

  MetricMetadata const* metric_metadata =
    Frontend::create_metric(metric_key, "requests_total", {{"method", "POST"}, {"status", "200"}});

  METRIC(logger, metric_metadata, 12.5);
  logger->publish_metric(metric_metadata, 3.25);
  logger->flush_log();

  Backend::stop();
  Frontend::remove_logger(logger);

  auto* sink_ptr = static_cast<MetricCapturingSink*>(metric_sink.get());
  REQUIRE_EQ(sink_ptr->log_writes.load(std::memory_order_relaxed), 0);

  std::lock_guard<std::mutex> const lock{sink_ptr->mutex};
  REQUIRE_EQ(sink_ptr->metrics.size(), 2);

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
}
