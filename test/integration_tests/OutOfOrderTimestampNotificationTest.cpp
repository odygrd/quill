#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/UserClockSource.h"
#include "quill/backend/RdtscClock.h"
#include "quill/core/ChronoTimeUtils.h"
#include "quill/core/Rdtsc.h"
#include "quill/sinks/FileSink.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

using namespace quill;

namespace
{
/** A clock that returns the timestamp the test sets, in the format it is created with */
class TestClockSource : public UserClockSource
{
public:
  explicit TestClockSource(TimestampType timestamp_type) : _timestamp_type(timestamp_type) {}

  uint64_t now() const noexcept override { return _timestamp.load(std::memory_order_relaxed); }

  TimestampType timestamp_type() const noexcept override { return _timestamp_type; }

  void set_timestamp(uint64_t timestamp) { _timestamp.store(timestamp, std::memory_order_relaxed); }

private:
  std::atomic<uint64_t> _timestamp{0};
  TimestampType _timestamp_type;
};

/** The number of ticks in half a microsecond more than `microseconds` */
uint64_t ticks_of_microseconds_and_a_half(uint64_t microseconds)
{
  double const nanoseconds = (static_cast<double>(microseconds) * 1'000.0) + 500.0;
  return static_cast<uint64_t>(nanoseconds / detail::RdtscClock::RdtscTicks::instance().ns_per_tick());
}
} // namespace

TEST_CASE("out_of_order_timestamp_notification")
{
  static constexpr char const* filename = "out_of_order_timestamp_notification.log";
  static std::string const logger_name = "out_of_order_timestamp_notification_logger";
  static std::string const epoch_logger_name = "out_of_order_timestamp_notification_epoch_logger";

  std::vector<std::string> notifications;

  BackendOptions backend_options;

  // The backend converts every record with the same clock base, so that the difference of two
  // timestamps is the difference the test sets
  backend_options.rdtsc_resync_interval = std::chrono::hours{1};

  backend_options.error_notifier = [&notifications](std::string const& error_message)
  {
    if (error_message.find("out of timestamp order") != std::string::npos)
    {
      notifications.push_back(error_message);
    }
  };

  Backend::start(backend_options);

  auto file_sink = Frontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  // A custom clock in epoch nanoseconds is authoritative: its records are not held back for the
  // grace period, so nothing is reported when it moves backwards
  TestClockSource epoch_clock{TimestampType::EpochNanoseconds};
  Logger* epoch_logger = Frontend::create_or_get_logger(
    epoch_logger_name, file_sink, PatternFormatterOptions{"%(message)"}, ClockSource::custom(epoch_clock));

  epoch_clock.set_timestamp(2'000'000'000);
  LOG_INFO(epoch_logger, "epoch clock");

  epoch_clock.set_timestamp(1'000'000'000);
  LOG_INFO(epoch_logger, "epoch clock moved backwards");

  // The backend converts TSC ticks to wall time and orders the records with the grace period,
  // as it does for the built-in clocks
  TestClockSource clock{TimestampType::TscTicks};
  Logger* logger = Frontend::create_or_get_logger(
    logger_name, file_sink, PatternFormatterOptions{"%(message)"}, ClockSource::custom(clock));

  // Every late record is older than this one by the given microseconds and a half
  uint64_t const newest_ticks = detail::rdtsc();

  clock.set_timestamp(newest_ticks);
  LOG_INFO(logger, "newest");

  // A future simulation timestamp must not make the next real-time record look late.
  epoch_clock.set_timestamp(detail::get_system_time_ns() + 60'000'000'000u);
  LOG_INFO(epoch_logger, "simulation in the future");

  // Less than a microsecond late: nothing to report
  clock.set_timestamp(newest_ticks - ticks_of_microseconds_and_a_half(0));
  LOG_INFO(logger, "late by less than a microsecond");

  // The default grace period of 5 us and 571 us make 576 us, reported as 600 us
  clock.set_timestamp(newest_ticks - ticks_of_microseconds_and_a_half(571));
  LOG_INFO(logger, "late by 571 us");

  // 5 us and 595 us make 600 us, which was reported
  clock.set_timestamp(newest_ticks - ticks_of_microseconds_and_a_half(595));
  LOG_INFO(logger, "late by 595 us");

  // A record that is in order changes nothing
  clock.set_timestamp(newest_ticks);
  LOG_INFO(logger, "in order");

  // 5 us and 1228 us make 1233 us, reported as 2000 us
  clock.set_timestamp(newest_ticks - ticks_of_microseconds_and_a_half(1228));
  LOG_INFO(logger, "late by 1228 us");

  Backend::stop();

  REQUIRE_EQ(notifications.size(), 2);

  REQUIRE_NE(notifications[0].find("Quill WARNING: A record is 571 us out of timestamp order"),
             std::string::npos);
  REQUIRE_NE(notifications[0].find("log_timestamp_ordering_grace_period to at least 600 us"), std::string::npos);

  REQUIRE_NE(notifications[1].find("Quill WARNING: A record is 1228 us out of timestamp order"),
             std::string::npos);
  REQUIRE_NE(notifications[1].find("log_timestamp_ordering_grace_period to at least 2000 us"), std::string::npos);

  notifications.clear();

  auto const write_late_pair = [&]()
  {
    uint64_t const ticks = detail::rdtsc();
    clock.set_timestamp(ticks);
    LOG_INFO(logger, "newest after the restart");
    clock.set_timestamp(ticks - ticks_of_microseconds_and_a_half(5000));
    LOG_INFO(logger, "late by 5000 us");
  };

  backend_options.log_timestamp_ordering_grace_period = std::chrono::microseconds{0};
  Backend::start(backend_options);
  write_late_pair();
  Backend::stop();
  REQUIRE(notifications.empty());

  // A corrected timestamp does not produce an out-of-order warning either.
  backend_options.log_timestamp_ordering_grace_period = std::chrono::microseconds{5};
  backend_options.ensure_monotonic_output_timestamps = true;
  Backend::start(backend_options);
  write_late_pair();
  Frontend::remove_logger(logger);
  Frontend::remove_logger(epoch_logger);
  Backend::stop();
  REQUIRE(notifications.empty());

  // The records are written as they are, in the order of the calls
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 13);
  REQUIRE_EQ(file_contents[1], "epoch clock moved backwards");
  REQUIRE_EQ(file_contents[2], "newest");
  REQUIRE_EQ(file_contents[5], "late by 571 us");
  REQUIRE_EQ(file_contents[10], "late by 5000 us");

  testing::remove_file(filename);
}
