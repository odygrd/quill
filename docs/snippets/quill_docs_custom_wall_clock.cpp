#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/ConsoleSink.h"

#include <chrono>
#include <cstdint>
#include <memory>

// A clock that follows real time, such as a PTP or NIC hardware clock. This sample reads
// std::chrono::system_clock in its place.
struct WallClock : quill::UserClockSource
{
  uint64_t now() const noexcept override
  {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                   std::chrono::system_clock::now().time_since_epoch())
                                   .count());
  }

  // The records wait for the grace period and are ordered with those of the other loggers
  quill::TimestampType timestamp_type() const noexcept override
  {
    return quill::TimestampType::WallClockNanoseconds;
  }
};

int main()
{
  WallClock clock;
  quill::Backend::start();
  std::shared_ptr<quill::Sink> sink =
    quill::Frontend::create_or_get_sink<quill::ConsoleSink>("custom_wall_clock_console");
  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "custom_wall_clock_logger", sink, quill::PatternFormatterOptions{}, quill::ClockSource::custom(clock));

  LOG_INFO(logger, "Timestamp read from a wall clock");

  logger->flush_log();
  quill::Frontend::remove_logger_blocking(logger);
  quill::Backend::stop();
}
