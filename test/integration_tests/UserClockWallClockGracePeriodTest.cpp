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
/** Returns system time, reported as the given timestamp type */
class WallClockGracePeriodClock : public UserClockSource
{
public:
  explicit WallClockGracePeriodClock(TimestampType timestamp_type) : _timestamp_type(timestamp_type)
  {
  }

  uint64_t now() const noexcept override
  {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                   std::chrono::system_clock::now().time_since_epoch())
                                   .count());
  }

  TimestampType timestamp_type() const noexcept override { return _timestamp_type; }

private:
  TimestampType const _timestamp_type;
};

/** Keeps the records in the order the backend writes them */
class WallClockGracePeriodSink final : public Sink
{
public:
  void write_log(LogRecord const& record) override { records.emplace_back(record.log_statement); }

  void flush_sink(SinkFlushReason /* flush_reason */) override {}

  std::vector<std::string> records;
};
} // namespace

/**
 * The record of a user clock that returns TimestampType::WallClockNanoseconds waits for the grace
 * period like a record of the system clock. The same clock returning
 * TimestampType::EpochNanoseconds keeps its own timeline, and its record is written at once.
 */
TEST_CASE("user_clock_wall_clock_grace_period")
{
  static std::string const logger_name = "user_clock_wall_clock_grace_period_logger";

  // A single event for each poll
  BackendOptions backend_options;
  backend_options.sleep_duration = std::chrono::nanoseconds{0};

  // The record of the wall clock waits for the whole test
  backend_options.log_timestamp_ordering_grace_period = std::chrono::seconds{60};

  ManualBackendWorker* manual_backend_worker = Backend::acquire_manual_backend_worker();
  manual_backend_worker->init(backend_options);

  auto sink = Frontend::create_or_get_sink<WallClockGracePeriodSink>(
    "user_clock_wall_clock_grace_period_sink");

  WallClockGracePeriodClock wall_clock{TimestampType::WallClockNanoseconds};
  Logger* wall_clock_logger = Frontend::create_or_get_logger(
    logger_name + "_wall_clock", sink, PatternFormatterOptions{"%(message)"}, ClockSource::custom(wall_clock));

  WallClockGracePeriodClock epoch_clock{TimestampType::EpochNanoseconds};
  Logger* epoch_clock_logger = Frontend::create_or_get_logger(logger_name + "_epoch_clock", sink,
                                                              PatternFormatterOptions{"%(message)"},
                                                              ClockSource::custom(epoch_clock));

  std::thread wall_clock_thread{[wall_clock_logger]()
                                { LOG_INFO(wall_clock_logger, "wall clock record"); }};
  wall_clock_thread.join();

  manual_backend_worker->poll_one();
  manual_backend_worker->poll_one();
  manual_backend_worker->poll_one();
  std::vector<std::string> const records_after_wall_clock_polls = sink->records;

  // Logged from another thread, so its record is not behind the waiting one in the same queue
  std::thread epoch_clock_thread{[epoch_clock_logger]()
                                 { LOG_INFO(epoch_clock_logger, "epoch clock record"); }};
  epoch_clock_thread.join();

  manual_backend_worker->poll_one();
  manual_backend_worker->poll_one();
  std::vector<std::string> const records_after_epoch_clock_polls = sink->records;

  // The shutdown reads the waiting record without the grace period. It comes before the checks,
  // as the backend reads the clocks until it stops
  Frontend::remove_logger(wall_clock_logger);
  Frontend::remove_logger(epoch_clock_logger);
  manual_backend_worker->shutdown();

  REQUIRE(records_after_wall_clock_polls.empty());

  REQUIRE_EQ(records_after_epoch_clock_polls.size(), 1);
  REQUIRE_EQ(records_after_epoch_clock_polls[0], std::string{"epoch clock record\n"});

  REQUIRE_EQ(sink->records.size(), 2);
  REQUIRE_EQ(sink->records[1], std::string{"wall clock record\n"});
}
