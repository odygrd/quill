#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"

#include <atomic>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
// The selected type can be abstract while providing the timestamp read implementation.
struct MetricClock : quill::UserClockSource
{
  virtual void set_timestamp(uint64_t value) = 0;

  uint64_t now() const noexcept override
  {
    reads.fetch_add(1, std::memory_order_relaxed);
    return timestamp.load(std::memory_order_relaxed);
  }

  std::atomic<uint64_t> timestamp{0};
  mutable std::atomic<size_t> reads{0};
};

struct TestMetricClock final : MetricClock
{
  void set_timestamp(uint64_t value) override { timestamp.store(value, std::memory_order_relaxed); }
};

struct MetricClockOptions : quill::FrontendOptions
{
  using clock_type = MetricClock;
};

struct CapturedMetric
{
  uint64_t timestamp;
  double value;
};

struct MetricClockSink : quill::Sink
{
  void write_log(quill::LogRecord const& /* record */) override
  {
  }

  void write_metric(quill::MetricRecord const& record) override
  {
    metrics.push_back({record.timestamp, record.value});
  }

  void flush_sink(quill::SinkFlushReason) override {}

  std::vector<CapturedMetric> metrics;
};
} // namespace

TEST_CASE("frontend_clock_metrics")
{
  using MetricFrontend = quill::FrontendImpl<MetricClockOptions>;
  TestMetricClock clock;
  auto sink = MetricFrontend::create_sink<MetricClockSink>("frontend_clock_metric_sink");
  MetricClockSink* capture = static_cast<MetricClockSink*>(sink.get());
  MetricFrontend::logger_t* logger = MetricFrontend::create_logger(
    "frontend_clock_metric_logger", sink, quill::PatternFormatterOptions{}, quill::ClockSource::custom(clock));
  quill::MetricMetadata const* metadata =
    MetricFrontend::create_metric("frontend_clock_metric", "frontend_clock_metric");

  // The first publish initializes the queue. Its valid zero timestamp must not be read again.
  REQUIRE(logger->publish_metric(metadata, 0.5));
  CHECK_EQ(clock.reads.load(), 1);

  // Dynamic metrics use the same clock instance and preserve its updated nanosecond timestamp.
  clock.set_timestamp(1'000'000'000);
  REQUIRE(logger->publish_dynamic_metric("frontend_clock_dynamic_metric", {}, 1.5));
  CHECK_EQ(clock.reads.load(), 2);

  quill::Backend::start();
  logger->flush_log();
  MetricFrontend::remove_logger_blocking(logger);
  quill::Backend::stop();

  REQUIRE_EQ(capture->metrics.size(), 2);
  CHECK_EQ(capture->metrics[0].timestamp, 0);
  CHECK_EQ(capture->metrics[0].value, doctest::Approx{0.5});
  CHECK_EQ(capture->metrics[1].timestamp, 1'000'000'000);
  CHECK_EQ(capture->metrics[1].value, doctest::Approx{1.5});
}
