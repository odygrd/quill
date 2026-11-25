#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/UserClockSource.h"
#include "quill/sinks/Sink.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

using namespace quill;

namespace
{
/** A clock of the epoch, such as the time of a simulation, that runs an hour ahead of system time */
class GracePeriodAheadClock : public UserClockSource
{
public:
  uint64_t now() const noexcept override
  {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                   (std::chrono::system_clock::now() + std::chrono::hours{1}).time_since_epoch())
                                   .count());
  }
};

/** Keeps the records in the order the backend writes them */
class GracePeriodAheadSink final : public Sink
{
public:
  void write_log(LogRecord const& record) override { records.emplace_back(record.log_statement); }

  void flush_sink(SinkFlushReason /* flush_reason */) override {}

  std::vector<std::string> records;
};
} // namespace

/**
 * The record of a clock of the epoch that runs ahead of system time does not wait for the grace
 * period, also while a record of the system clock waits for it: a poll that writes a single event
 * writes it. The timestamp limit that a batch keeps while a record waits ends with the batch, so it
 * does not hold this record back for as long as its clock is ahead.
 */
TEST_CASE("backend_grace_period_custom_clock_ahead")
{
  static std::string const logger_name = "backend_grace_period_custom_clock_ahead_logger";

  // A single event for each poll
  BackendOptions backend_options;
  backend_options.sleep_duration = std::chrono::nanoseconds{0};

  // The record of the system clock waits for the whole test
  backend_options.log_timestamp_ordering_grace_period = std::chrono::seconds{60};

  ManualBackendWorker* manual_backend_worker = Backend::acquire_manual_backend_worker();
  manual_backend_worker->init(backend_options);

  auto sink = Frontend::create_or_get_sink<GracePeriodAheadSink>(
    "backend_grace_period_custom_clock_ahead_sink");

  Logger* system_clock_logger = Frontend::create_or_get_logger(
    logger_name + "_system_clock", sink, PatternFormatterOptions{"%(message)"}, ClockSource::system());

  GracePeriodAheadClock ahead_clock;
  Logger* ahead_logger = Frontend::create_or_get_logger(
    logger_name + "_ahead", sink, PatternFormatterOptions{"%(message)"}, ClockSource::custom(ahead_clock));

  std::thread waiting_thread{[system_clock_logger]()
                             { LOG_INFO(system_clock_logger, "waiting record"); }};
  waiting_thread.join();

  manual_backend_worker->poll_one();

  std::thread ahead_thread{[ahead_logger]() { LOG_INFO(ahead_logger, "ahead record"); }};
  ahead_thread.join();

  manual_backend_worker->poll_one();
  manual_backend_worker->poll_one();
  std::vector<std::string> const records_after_polls = sink->records;

  // The shutdown reads the waiting record without the grace period. It comes before the checks,
  // as the backend reads the ahead clock until it stops
  Frontend::remove_logger(system_clock_logger);
  Frontend::remove_logger(ahead_logger);
  manual_backend_worker->shutdown();

  REQUIRE_EQ(records_after_polls.size(), 1);
  REQUIRE_EQ(records_after_polls[0], std::string{"ahead record\n"});

  REQUIRE_EQ(sink->records.size(), 2);
  REQUIRE_EQ(sink->records[1], std::string{"waiting record\n"});
}
