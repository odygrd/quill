#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/core/Rdtsc.h"
#include "quill/sinks/ConsoleSink.h"

#include <cstdint>
#include <memory>

struct TscClock : quill::UserClockSource
{
  uint64_t now() const noexcept override { return quill::detail::rdtsc(); }

  quill::TimestampType timestamp_type() const noexcept override
  {
    return quill::TimestampType::TscTicks;
  }
};

struct TscFrontendOptions : quill::FrontendOptions
{
  using clock_type = TscClock;
};

using TscFrontend = quill::FrontendImpl<TscFrontendOptions>;
using TscLogger = TscFrontend::logger_t;

int main()
{
  TscClock clock;
  quill::Backend::start();
  std::shared_ptr<quill::Sink> sink =
    TscFrontend::create_or_get_sink<quill::ConsoleSink>("custom_tsc_clock_console");
  TscLogger* logger = TscFrontend::create_or_get_logger(
    "custom_tsc_clock_logger", sink, quill::PatternFormatterOptions{}, quill::ClockSource::custom(clock));

  LOG_INFO(logger, "Timestamp captured as raw TSC ticks");

  logger->flush_log();
  TscFrontend::remove_logger_blocking(logger);
  quill::Backend::stop();
}
