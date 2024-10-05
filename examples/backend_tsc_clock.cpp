#include "quill/Backend.h"
#include "quill/BackendTscClock.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

#include <iostream>
#include <thread>

/**
 * This example demonstrates how to retrieve a timestamp in your application that is synchronized
 * with the timestamps used in the log files, when a Logger with TSC (Time Stamp Counter)
 * is being used as the clock source for logging in the backend.
 *
 * The backend thread runs independently using its own TSC clock for timestamping log entries.
 */

int main()
{
  // Start the backend thread to process logs and use TSC clock for logging
  quill::Backend::start();

  // Create or get a logger with a console sink, using TSC as the clock source which is already the default
  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "root", quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1"),
    quill::PatternFormatterOptions{}, quill::ClockSourceType::Tsc);

  // Log an example message to initialize the backend and timestamp the log
  LOG_INFO(logger, "Backend initialized");

  // Ensure the backend has processed at least one log since the application start
  logger->flush_log();

  // Log both the backend TSC clock epoch time and the current system clock epoch time
  auto const backend_tsc_epoch_ns = quill::BackendTscClock::now().time_since_epoch().count();
  auto const system_epoch_ns = std::chrono::system_clock::now().time_since_epoch().count();

  LOG_INFO(logger, "Backend TSC clock epoch time: {} ns, System clock epoch time: {} ns",
           backend_tsc_epoch_ns, system_epoch_ns);

  // Get the current TSC timestamp now
  auto const tsc_start = quill::BackendTscClock::rdtsc();

  // Introduce a delay to simulate some work
  std::this_thread::sleep_for(std::chrono::seconds{2});

  // Get another TSC timestamp after the delay
  auto const tsc_end = quill::BackendTscClock::rdtsc();

  // Convert the TSC timestamps to seconds since the epoch
  auto const tsc_start_seconds = std::chrono::duration_cast<std::chrono::seconds>(
                                   quill::BackendTscClock::to_time_point(tsc_start).time_since_epoch())
                                   .count();

  auto const tsc_end_seconds = std::chrono::duration_cast<std::chrono::seconds>(
                                 quill::BackendTscClock::to_time_point(tsc_end).time_since_epoch())
                                 .count();

  // Log the TSC timestamps before and after the delay
  LOG_INFO(logger, "Initial TSC timestamp: {} seconds, Final TSC timestamp after delay: {} seconds",
           tsc_start_seconds, tsc_end_seconds);
}