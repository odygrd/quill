#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/core/TimeUtilities.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <ctime>
#include <string>
#include <vector>

using namespace quill;

/**
 * Custom timestamp class
 */
class UserClockSourceTest : public UserClockSource
{
public:
  explicit UserClockSourceTest(TimestampType format = TimestampType::EpochNanoseconds)
    : _timestamp_type(format)
  {
  }

  TimestampType timestamp_type() const noexcept override { return _timestamp_type; }

  uint64_t now() const override
  {
    reads.fetch_add(1, std::memory_order_relaxed);
    return _timestamp_type == TimestampType::TscTicks ? quill::detail::rdtsc() : _ts.load();
  }

  /**
   * set custom timestamp
   * @param time_since_epoch timestamp
   */
  void set_timestamp(std::chrono::seconds time_since_epoch)
  {
    // always convert to nanos
    _ts.store(static_cast<uint64_t>(std::chrono::nanoseconds{time_since_epoch}.count()));
  }

  mutable std::atomic<size_t> reads{0};

private:
  TimestampType const _timestamp_type;

  /**
   * time since epoch - must always be in nanoseconds
   * This class needs to be thread-safe, unless only a single thread in the application calling LOG macros
   * **/
  std::atomic<uint64_t> _ts{0};
};

/***/
TEST_CASE("user_clock_source")
{
  std::time_t const before = std::time(nullptr);
  static constexpr size_t number_of_messages = 10;
  static constexpr char const* filename = "user_clock_source.log";
  static std::string const logger_name = "logger";

  // Start the logging backend thread
  Backend::start();

  UserClockSourceTest uct;
  uct.set_timestamp(std::chrono::seconds{1655007309});

  // Set writing logging to a file
  auto file_sink = Frontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger(
    logger_name, std::move(file_sink),
    PatternFormatterOptions{"%(time) %(log_level) %(logger:<16) %(message)", // format
                            "%Y-%m-%d %H:%M:%S.%Qms",                        // timestamp format
                            quill::Timezone::GmtTime, false},
    ClockSource::custom(uct));

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    // log an array so the log message is pushed to the queue
    LOG_INFO(logger, "Lorem ipsum dolor sit amet, consectetur adipiscing elit {}", i);
  }

  uct.set_timestamp(std::chrono::seconds{1656007309});

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    LOG_ERROR(logger, "Nulla tempus, libero at dignissim viverra, lectus libero finibus ante {}", i);
  }

  // Runtime dispatch reads the supplied epoch/TSC clock objects rather than built-in clocks.
  UserClockSourceTest epoch_clock;
  epoch_clock.set_timestamp(std::chrono::seconds{1655007309});
  Logger* epoch_logger =
    Frontend::create_logger("user_clock_source_epoch", logger->get_sinks().front(),
                            logger->get_pattern_formatter_options(), ClockSource::custom(epoch_clock));
  LOG_INFO(epoch_logger, "custom epoch");
  CHECK_EQ(epoch_clock.reads.load(), 1);
  CHECK_EQ(epoch_logger->get_clock_source_type(), ClockSourceType::User);

  UserClockSourceTest tsc_clock{TimestampType::TscTicks};
  Logger* tsc_logger =
    Frontend::create_logger("user_clock_source_tsc", logger->get_sinks().front(),
                            logger->get_pattern_formatter_options(), ClockSource::custom(tsc_clock));
  LOG_INFO(tsc_logger, "custom tsc");
  CHECK_EQ(tsc_clock.reads.load(), 1);
  CHECK_EQ(tsc_logger->get_clock_source_type(), ClockSourceType::User);

  // Let all log get flushed to the file
  uct.set_timestamp(std::chrono::seconds{1658007309});
  logger->flush_log();
  std::time_t const after = std::time(nullptr);
  Frontend::remove_logger(logger);
  Frontend::remove_logger(epoch_logger);
  Frontend::remove_logger(tsc_logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), number_of_messages * 2 + 2);

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    std::string expected_string = "2022-06-12 04:15:09.000 INFO " + logger_name +
      "           Lorem ipsum dolor sit amet, consectetur adipiscing elit " + std::to_string(i);
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));
  }

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    std::string expected_string = "2022-06-23 18:01:49.000 ERROR " + logger_name +
      "           Nulla tempus, libero at dignissim viverra, lectus libero finibus ante " +
      std::to_string(i);
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));
  }

  CHECK(testing::file_contains(
    file_contents, "2022-06-12 04:15:09.000 INFO user_clock_source_epoch custom epoch"));

  std::tm before_tm;
  std::tm after_tm;
  detail::gmtime_rs(&before, &before_tm);
  detail::gmtime_rs(&after, &after_tm);
  std::string const tsc_year = file_contents.back().substr(0, 4);
  CHECK((tsc_year == std::to_string(before_tm.tm_year + 1900) ||
         tsc_year == std::to_string(after_tm.tm_year + 1900)));
  CHECK(testing::file_contains(file_contents, "custom tsc"));

  testing::remove_file(filename);
}