#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using namespace quill;

namespace
{
class TestUserClockSource : public UserClockSource
{
public:
  uint64_t now() const override { return _timestamp.load(std::memory_order_relaxed); }

  void set_timestamp(std::chrono::nanoseconds timestamp)
  {
    _timestamp.store(static_cast<uint64_t>(timestamp.count()), std::memory_order_relaxed);
  }

private:
  std::atomic<uint64_t> _timestamp{0};
};

Logger* create_user_clock_file_logger(std::string const& logger_name, char const* filename,
                                      TestUserClockSource& user_clock)
{
  auto file_sink = Frontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  return Frontend::create_or_get_logger(
    logger_name, std::move(file_sink),
    PatternFormatterOptions{"%(time) %(log_level) %(message)", "%Y-%m-%d %H:%M:%S.%Qns", Timezone::GmtTime, false},
    ClockSourceType::User, &user_clock);
}
} // namespace

TEST_CASE("monotonic_output_timestamps")
{
  static constexpr char const* filename = "monotonic_output_timestamps.log";
  static std::string const logger_name = "monotonic_output_timestamps_logger";

  BackendOptions backend_options;
  backend_options.ensure_monotonic_output_timestamps = true;
  Backend::start(backend_options);

  TestUserClockSource user_clock;
  Logger* logger = create_user_clock_file_logger(logger_name, filename, user_clock);
  logger->init_backtrace(1);

  user_clock.set_timestamp(std::chrono::seconds{2});
  LOG_INFO(logger, "first");

  user_clock.set_timestamp(std::chrono::seconds{1});
  LOG_INFO(logger, "second");
  LOG_BACKTRACE(logger, "historical");
  logger->flush_backtrace();

  Backend::stop();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 3);
  REQUIRE_EQ(file_contents[0], "1970-01-01 00:00:02.000000000 INFO first");
  REQUIRE_EQ(file_contents[1], "1970-01-01 00:00:02.000000001 INFO second");
  REQUIRE_EQ(file_contents[2], "1970-01-01 00:00:01.000000000 BACKTRACE historical");

  testing::remove_file(filename);
}
