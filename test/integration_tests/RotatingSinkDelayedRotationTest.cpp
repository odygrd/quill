#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/UserClockSource.h"
#include "quill/sinks/RotatingFileSink.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <fstream>
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

std::string format_datetime_suffix(std::chrono::nanoseconds timestamp)
{
  auto const seconds = std::chrono::duration_cast<std::chrono::seconds>(timestamp).count();
  std::time_t t = static_cast<std::time_t>(seconds);
  std::tm tm_buf{};
#if defined(_WIN32)
  ::gmtime_s(&tm_buf, &t);
#else
  ::gmtime_r(&t, &tm_buf);
#endif
  char buf[32];
  std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tm_buf);
  return std::string{buf};
}
} // namespace

/**
 * Regression test for time-based rotation when the logger goes silent across a rotation
 * boundary. The rotated file's name must use the original `_open_file_timestamp` (the
 * timestamp the closed file's data started at), not the late-arriving log's timestamp
 * — and the next-rotation timer must advance to the future, not stick at the missed
 * boundary, so we don't redundantly rotate on every subsequent write.
 */
TEST_CASE("rotating_sink_delayed_rotation")
{
  static constexpr char const* base_filename = "rotating_sink_delayed_rotation.log";
  static std::string const logger_name = "rotating_sink_delayed_rotation_logger";

  TestUserClockSource user_clock;

  // Start at a known epoch — pick a value with even seconds so the boundary math is easy.
  // 2026-01-02 12:00:00 UTC = 1767355200 seconds
  std::chrono::nanoseconds const t0{std::chrono::seconds{1767355200}};
  user_clock.set_timestamp(t0);

  BackendOptions bo;
  bo.check_printable_char = {};
  Backend::start(bo);

  // Anchor the sink's rotation calculations to the same user-clock time as our first log
  // so the rotation boundary lines up with the simulated quiet period.
  auto const start_time = std::chrono::system_clock::time_point{
    std::chrono::duration_cast<std::chrono::system_clock::duration>(t0)};

  std::shared_ptr<quill::Sink> rotating_sink = Frontend::create_or_get_sink<RotatingFileSink>(
    base_filename,
    []()
    {
      RotatingFileSinkConfig cfg;
      cfg.set_rotation_frequency_and_interval('M', 1);
      cfg.set_rotation_naming_scheme(RotatingFileSinkConfig::RotationNamingScheme::DateAndTime);
      cfg.set_open_mode('w');
      cfg.set_timezone(Timezone::GmtTime);
      return cfg;
    }(),
    FileEventNotifier{}, start_time);

  Logger* logger = Frontend::create_or_get_logger(
    logger_name, std::move(rotating_sink),
    PatternFormatterOptions{"%(message)", "%H:%M:%S.%Qns", Timezone::GmtTime, false},
    ClockSourceType::User, &user_clock);

  // First log at t0 — opens the file with _open_file_timestamp = t0
  LOG_INFO(logger, "first at t0");
  logger->flush_log();

  // Now simulate a quiet period: advance the clock past several rotation boundaries
  // (12 minutes later). No log statements happen in this gap.
  std::chrono::nanoseconds const t_late = t0 + std::chrono::minutes{12};
  user_clock.set_timestamp(t_late);

  // A log finally arrives. _time_rotation will fire because t_late >= _next_rotation_time.
  // The rotated file should be named with the t0 timestamp suffix (the data IS from t0),
  // NOT with the t_late suffix.
  LOG_INFO(logger, "late after quiet period");
  logger->flush_log();

  // Another log at t_late + small delta — must not trigger another rotation. The next
  // rotation boundary must be in the future relative to t_late, not stuck at a missed
  // boundary between t0 and t_late.
  std::chrono::nanoseconds const t_late2 = t_late + std::chrono::seconds{5};
  user_clock.set_timestamp(t_late2);
  LOG_INFO(logger, "shortly after");
  logger->flush_log();

  Frontend::remove_logger(logger);
  Backend::stop();

  // The rotated file should have a name like:
  //   rotating_sink_delayed_rotation.20260102_120000.log
  std::string const expected_suffix = format_datetime_suffix(t0);
  std::string const rotated_filename =
    std::string{"rotating_sink_delayed_rotation."} + expected_suffix + std::string{".log"};

  // The rotated filename uses the t0 timestamp (the data start time of the closed file).
  std::vector<std::string> const rotated_contents = quill::testing::file_contents(rotated_filename);
  REQUIRE_EQ(rotated_contents.size(), 1u);
  REQUIRE_EQ(rotated_contents[0], "first at t0");

  // The current file holds the late + post-late messages and must NOT have rotated again
  // for the second late log, otherwise we'd have a second rotated file.
  std::vector<std::string> const current_contents = quill::testing::file_contents(base_filename);
  REQUIRE_EQ(current_contents.size(), 2u);
  REQUIRE_EQ(current_contents[0], "late after quiet period");
  REQUIRE_EQ(current_contents[1], "shortly after");

  // Sanity check: no rotated file using t_late timestamp should exist (would indicate
  // the rotated filename was computed from record_timestamp_ns instead of _open_file_timestamp).
  std::string const wrong_suffix = format_datetime_suffix(t_late);
  std::string const wrong_filename =
    std::string{"rotating_sink_delayed_rotation."} + wrong_suffix + std::string{".log"};
  REQUIRE_FALSE(std::ifstream{wrong_filename}.good());

  testing::remove_file(base_filename);
  testing::remove_file(rotated_filename.c_str());
}
