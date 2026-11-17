#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogFunctions.h"
#include "quill/LogMacros.h"
#include "quill/core/TimeUtilities.h"
#include "quill/sinks/FileSink.h"

#include <atomic>
#include <ctime>
#include <string>
#include <utility>
#include <vector>

namespace
{
struct AppClock : quill::UserClockSource
{
  explicit AppClock(quill::TimestampType format = quill::TimestampType::EpochNanoseconds)
    : timestamp_format(format)
  {
  }

  uint64_t now() const noexcept override
  {
    timestamp_reads.fetch_add(1, std::memory_order_relaxed);
    return timestamp_format == quill::TimestampType::TscTicks ? quill::detail::rdtsc()
                                                              : timestamp.load(std::memory_order_relaxed);
  }

  quill::TimestampType timestamp_type() const noexcept override { return timestamp_format; }

  quill::TimestampType const timestamp_format;
  std::atomic<uint64_t> timestamp{0};
  mutable std::atomic<size_t> timestamp_reads{0};
};

struct AppOptions : quill::FrontendOptions
{
  using clock_type = AppClock;
  static constexpr size_t initial_queue_capacity = 1024;
};
} // namespace

TEST_CASE("frontend_clock_provider")
{
  std::time_t const before = std::time(nullptr);
  static constexpr char const* filename = "frontend_clock_provider.log";
  using AppFrontend = quill::FrontendImpl<AppOptions>;

  AppClock clock;
  AppClock second_clock;
  AppClock epoch_clock;
  AppClock tsc_clock{quill::TimestampType::TscTicks};

  quill::FileSinkConfig config;
  config.set_open_mode('w');
  auto sink = AppFrontend::create_or_get_sink<quill::FileSink>(filename, config);

  AppFrontend::logger_t* logger = AppFrontend::create_or_get_logger(
    "frontend_clock_provider_logger", sink,
    quill::PatternFormatterOptions{"%(time) %(message)", "%Y-%m-%d %H:%M:%S.%Qns", quill::Timezone::GmtTime},
    quill::ClockSource::custom(clock));

  CHECK_EQ(logger->get_clock_source_type(), quill::ClockSourceType::User);
  CHECK_EQ(logger->get_user_clock_source(), &clock);

  // A zero timestamp survives the cold queue path. Macro-free logging uses the same instance.
  LOG_INFO(logger, "zero timestamp");

  // Force queue growth with the backend stopped. Recovery must reuse the captured zero timestamp.
  size_t const initial_capacity = AppFrontend::get_thread_local_queue_capacity();
  std::string const payload(initial_capacity, 'x');
  LOG_INFO(logger, "queue growth {}", payload);
  CHECK_GT(AppFrontend::get_thread_local_queue_capacity(), initial_capacity);
  CHECK_EQ(clock.timestamp_reads.load(), 2);

  clock.timestamp.store(1'000'000'000, std::memory_order_relaxed);
  quill::info(logger, "macro-free {}", 42);

  CHECK_EQ(clock.timestamp_reads.load(), 3);

  // Cloning keeps the same clock object and concrete frontend type.
  AppFrontend::logger_t* cloned_logger =
    AppFrontend::create_or_get_logger("frontend_clock_cloned_logger", logger);
  CHECK_EQ(cloned_logger->get_user_clock_source(), &clock);
  LOG_INFO(cloned_logger, "cloned clock");
  CHECK_EQ(clock.timestamp_reads.load(), 4);

  // The frontend fixes the clock type, while each logger may refer to a different clock instance.
  second_clock.timestamp.store(2'000'000'000, std::memory_order_relaxed);
  AppFrontend::logger_t* second_logger = AppFrontend::create_logger(
    "frontend_clock_second_logger", sink, logger->get_pattern_formatter_options(),
    quill::ClockSource::custom(second_clock));
  LOG_INFO(second_logger, "second instance");

  CHECK_EQ(second_clock.timestamp_reads.load(), 1);
  CHECK_EQ(clock.timestamp_reads.load(), 4);

  // Epoch nanoseconds are preserved exactly, without TSC conversion.
  epoch_clock.timestamp.store(1'655'007'309'000'000'000ULL, std::memory_order_relaxed);
  AppFrontend::logger_t* epoch_logger = AppFrontend::create_logger(
    "frontend_clock_epoch_logger", sink, logger->get_pattern_formatter_options(),
    quill::ClockSource::custom(epoch_clock));
  LOG_INFO(epoch_logger, "epoch representation");

  CHECK_EQ(epoch_logger->get_clock_source_type(), quill::ClockSourceType::User);
  CHECK_EQ(epoch_clock.timestamp_reads.load(), 1);

  // Raw ticks must come from the supplied object, then be converted to wall time by the backend.
  AppFrontend::logger_t* tsc_logger =
    AppFrontend::create_logger("frontend_clock_tsc_logger", sink, logger->get_pattern_formatter_options(),
                               quill::ClockSource::custom(tsc_clock));
  LOG_INFO(tsc_logger, "tsc representation");

  CHECK_EQ(tsc_logger->get_clock_source_type(), quill::ClockSourceType::User);
  CHECK_EQ(tsc_clock.timestamp_reads.load(), 1);

  quill::Backend::start();
  tsc_logger->flush_log();
  std::time_t const after = std::time(nullptr);

  AppFrontend::remove_logger(logger);
  AppFrontend::remove_logger(cloned_logger);
  AppFrontend::remove_logger(second_logger);
  AppFrontend::remove_logger(epoch_logger);
  AppFrontend::remove_logger(tsc_logger);
  quill::Backend::stop();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 7);
  CHECK_EQ(file_contents[0], "1970-01-01 00:00:00.000000000 zero timestamp");
  CHECK_EQ(file_contents[1], "1970-01-01 00:00:00.000000000 queue growth " + payload);
  CHECK_EQ(file_contents[2], "1970-01-01 00:00:01.000000000 macro-free 42");

  CHECK_EQ(file_contents[3], "1970-01-01 00:00:01.000000000 cloned clock");
  CHECK_EQ(file_contents[4], "1970-01-01 00:00:02.000000000 second instance");
  CHECK_EQ(file_contents[5], "2022-06-12 04:15:09.000000000 epoch representation");

  std::tm before_tm;
  std::tm after_tm;
  quill::detail::gmtime_rs(&before, &before_tm);
  quill::detail::gmtime_rs(&after, &after_tm);
  std::string const tsc_year = file_contents.back().substr(0, 4);
  CHECK((tsc_year == std::to_string(before_tm.tm_year + 1900) ||
         tsc_year == std::to_string(after_tm.tm_year + 1900)));
  CHECK(quill::testing::file_contains(file_contents, "tsc representation"));

  quill::testing::remove_file(filename);
}
