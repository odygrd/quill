#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/ConsoleSink.h"

#include <atomic>
#include <cstdint>
#include <memory>

// Application-controlled epoch nanoseconds. A real application can supply its own clock read.
struct AppClock : quill::UserClockSource
{
  uint64_t now() const noexcept override { return timestamp_ns.load(std::memory_order_relaxed); }

  // Optional: epoch nanoseconds are also the base class default.
  quill::TimestampType timestamp_type() const noexcept override
  {
    return quill::TimestampType::EpochNanoseconds;
  }

  std::atomic<uint64_t> timestamp_ns{1655007309000000000ULL};
};

struct AppFrontendOptions : quill::FrontendOptions
{
  using clock_type = AppClock;
};

using AppFrontend = quill::FrontendImpl<AppFrontendOptions>;
using AppLogger = AppFrontend::logger_t;

int main()
{
  AppClock clock;
  quill::Backend::start();
  std::shared_ptr<quill::Sink> sink =
    AppFrontend::create_or_get_sink<quill::ConsoleSink>("custom_clock_console");
  AppLogger* logger = AppFrontend::create_or_get_logger(
    "custom_clock_logger", sink, quill::PatternFormatterOptions{}, quill::ClockSource::custom(clock));

  LOG_INFO(logger, "Application clock timestamp");
  clock.timestamp_ns.fetch_add(1000000000ULL, std::memory_order_relaxed);
  LOG_INFO(logger, "One second later");

  logger->flush_log();
  AppFrontend::remove_logger_blocking(logger);
  quill::Backend::stop();
}
