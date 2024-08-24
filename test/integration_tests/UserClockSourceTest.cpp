#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace quill;

/**
 * Custom timestamp class
 */
class UserClockSourceTest : public UserClockSource
{
public:
  UserClockSourceTest() = default;

  /**
   * Required by TimestampClock
   * @return current time now, in nanoseconds since epoch
   */
  uint64_t now() const override { return _ts.load(); }

  /**
   * set custom timestamp
   * @param time_since_epoch timestamp
   */
  void set_timestamp(std::chrono::seconds time_since_epoch)
  {
    // always convert to nanos
    _ts.store(static_cast<uint64_t>(std::chrono::nanoseconds{time_since_epoch}.count()));
  }

private:
  /**
   * time since epoch - must always be in nanoseconds
   * This class needs to be thread-safe, unless only a single thread in the application calling LOG macros
   * **/
  std::atomic<uint64_t> _ts;
};

/***/
TEST_CASE("user_clock_source")
{
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

  Logger* logger =
    Frontend::create_or_get_logger(logger_name, std::move(file_sink),
    PatternFormatterOptions{"%(time) %(log_level) %(logger:<16) %(message)", // format
                            "%Y-%m-%d %H:%M:%S.%Qms", // timestamp format
                            quill::Timezone::GmtTime, false},
    ClockSourceType::User, &uct);

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

  // Let all log get flushed to the file
  uct.set_timestamp(std::chrono::seconds{1658007309});
  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), number_of_messages * 2);

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

  testing::remove_file(filename);
}