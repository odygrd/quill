#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/UserClockSource.h"
#include "quill/sinks/ConsoleSink.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <utility>

/**
 * Example demonstrating custom clock usage for logging. This is particularly useful
 * when simulating events from a specific time period, allowing logs to align with
 * the simulated time.
 */

/**
 * This class needs to be thread-safe, unless only a single thread in the application calling
 * LOG_ macros from the same logger
 */
class SimulatedClock : public quill::UserClockSource
{
public:
  SimulatedClock() = default;

  /**
   * Required by TimestampClock
   * @return current time now, in nanoseconds since epoch
   */
  uint64_t now() const override { return _timestamp_ns.load(std::memory_order_relaxed); }

  /**
   * set custom timestamp
   * @param time_since_epoch timestamp
   */
  void set_timestamp(std::chrono::seconds time_since_epoch)
  {
    // always convert to nanos
    _timestamp_ns.store(static_cast<uint64_t>(std::chrono::nanoseconds{time_since_epoch}.count()),
                        std::memory_order_relaxed);
  }

private:
  std::atomic<uint64_t> _timestamp_ns{0}; // time since epoch - must always be in nanoseconds
};

int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Create a simulated timestamp class. Quill takes a pointer to this class,
  // and the user is responsible for its lifetime.
  // Ensure that the instance of this class remains alive for as long as the logger
  // object exists, until the logger is removed.
  SimulatedClock simulated_clock;

  // Get the console sink and also create a logger using the simulated clock
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");

  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "root", std::move(console_sink),
    quill::PatternFormatterOptions{
      "%(time) %(short_source_location:<28) LOG_%(log_level:<9) %(logger:<12) %(message)",
      "%D %H:%M:%S.%Qns", quill::Timezone::LocalTime, false},
    quill::ClockSourceType::User, &simulated_clock);

  // Change the LogLevel to print everything
  logger->set_log_level(quill::LogLevel::TraceL3);

  // Set our timestamp to Sunday 12 June 2022
  simulated_clock.set_timestamp(std::chrono::seconds{1655007309});
  LOG_TRACE_L3(logger, "This is a log trace l3 example {}", 1);
  LOG_TRACE_L2(logger, "This is a log trace l2 example {} {}", 2, 2.3);

  // update our timestamp
  simulated_clock.set_timestamp(std::chrono::seconds{1655039000});
  LOG_INFO(logger, "This is a log info {} example", "string");
  LOG_DEBUG(logger, "This is a log debug example {}", 4);
}