#include "quill/backend/StringFromTime.h"
#include "DocTestExtensions.h"
#include "doctest/doctest.h"
#include <cstdlib>
#include <ctime>
#include <limits>

TEST_SUITE_BEGIN("StringFromTime");

using namespace quill;
using namespace quill::detail;

#if !defined(_WIN32)
class ScopedTimezone
{
public:
  explicit ScopedTimezone(char const* timezone)
  {
    if (char const* current_tz = std::getenv("TZ"))
    {
      _previous_tz = current_tz;
      _had_tz = true;
    }

    setenv("TZ", timezone, 1);
    tzset();
  }

  ~ScopedTimezone()
  {
    if (_had_tz)
    {
      setenv("TZ", _previous_tz.c_str(), 1);
    }
    else
    {
      unsetenv("TZ");
    }
    tzset();
  }

  ScopedTimezone(ScopedTimezone const&) = delete;
  ScopedTimezone& operator=(ScopedTimezone const&) = delete;

private:
  std::string _previous_tz;
  bool _had_tz{false};
};
#endif

/***/
TEST_CASE("string_from_time_rejects_time_embedding_composite_modifiers")
{
#if !defined(QUILL_NO_EXCEPTIONS)
  // These modifiers embed the time of day but are not split into dynamic parts, so the displayed
  // time would silently freeze at the cached value between recalculations. init() must reject
  // them instead.
  for (auto const* fmt :
       {"%X", "%EX", "%c", "%Ec", "%OH", "%OI", "%OM", "%OS", "prefix %H:%M %c suffix"})
  {
    StringFromTime string_from_time;
    REQUIRE_THROWS_AS(string_from_time.init(fmt, Timezone::GmtTime), QuillError);
  }

  // escaped variants are literal text and must remain accepted
  for (auto const* fmt : {"literal %%X", "literal %%EX", "literal %%c", "literal %%Ec",
                          "literal %%OH", "literal %%OI", "literal %%OM", "literal %%OS"})
  {
    StringFromTime string_from_time;
    string_from_time.init(fmt, Timezone::GmtTime);
  }
#endif
}

/***/
TEST_CASE("string_from_time_localtime_format_time")
{
  std::string fmt2 = "%H:%M:%S";
  StringFromTime string_from_time;

  string_from_time.init(fmt2, Timezone::LocalTime);

  // Get the timestamp now
  time_t raw_ts;
  std::time(&raw_ts);

  // Try for a few timestamps
  for (uint32_t i = 0; i <= 500'000; ++i)
  {
    // Get the time from string from time
    auto const& time_s1 = string_from_time.format_timestamp(raw_ts);

    // Get the time from strftime
    std::tm time_info{};
    quill::detail::localtime_rs(&raw_ts, &time_info);

    char buffer[256];
    std::strftime(buffer, 256, fmt2.data(), &time_info);
    auto const time_s2 = std::string{buffer};

    REQUIRE_STREQ(time_s1.data(), time_s2.data());

    // Increment the timestamp for the next loop
    raw_ts += 1;
  }
}

/***/
TEST_CASE("string_from_time_localtime_format_I")
{
  std::string fmt2 = "%I:%M:%S%p";
  StringFromTime string_from_time;

  string_from_time.init(fmt2, Timezone::LocalTime);

  // Get the timestamp now
  time_t raw_ts;
  std::time(&raw_ts);

  // Try for a few timestamps
  for (uint32_t i = 0; i <= 500'000; ++i)
  {
    // Get the time from string from time
    auto const& time_s1 = string_from_time.format_timestamp(raw_ts);

    // Get the time from strftime
    std::tm time_info{};
    quill::detail::localtime_rs(&raw_ts, &time_info);
    char buffer[256];
    std::strftime(buffer, 256, fmt2.data(), &time_info);
    auto const time_s2 = std::string{buffer};

    REQUIRE_STREQ(time_s1.data(), time_s2.data());

    // Increment the timestamp for the next loop
    raw_ts += 1;
  }
}

/***/
TEST_CASE("string_from_time_localtime_fallback_to_strftime")
{
  // In this edge case we pass a timestamp that is back in time from our cached timestamp value.

  // Get the timestamp now
  time_t raw_ts;
  std::time(&raw_ts);

  // This will create the StringFromTime class and pre-format a string for the timestamp now
  std::string fmt2 = "%Y-%m-%dT%H:%M:%SZ";
  StringFromTime string_from_time;

  string_from_time.init(fmt2, Timezone::GmtTime);

  // Ask StringFromTime to format timestamps in the past

  // Try for a few timestamps
  for (uint32_t i = 0; i <= 500'000; ++i)
  {
    // Get the time from string from time
    auto const& time_s1 = string_from_time.format_timestamp(raw_ts);

    // Get the time from strftime
    std::tm time_info{};
    quill::detail::gmtime_rs(&raw_ts, &time_info);
    char buffer[256];
    std::strftime(buffer, 256, fmt2.data(), &time_info);
    auto const time_s2 = std::string{buffer};

    REQUIRE_STREQ(time_s1.data(), time_s2.data());

    // Decrement the timestamp for the next loop
    raw_ts -= 1;
  }
}

/***/
TEST_CASE("string_from_time_localtime_main_format")
{
  std::string fmt2 = "%Y-%m-%dT%H:%M:%SZ";
  StringFromTime string_from_time;

  string_from_time.init(fmt2, Timezone::LocalTime);

  // Get the timestamp now
  time_t raw_ts;
  std::time(&raw_ts);

  // Try for a few timestamps
  for (uint32_t i = 0; i <= 600'000; ++i)
  {
    // Get the time from string from time
    auto const& time_s1 = string_from_time.format_timestamp(raw_ts);

    // Get the time from strftime
    std::tm time_info{};
    quill::detail::localtime_rs(&raw_ts, &time_info);
    char buffer[256];
    std::strftime(buffer, 256, fmt2.data(), &time_info);
    auto const time_s2 = std::string{buffer};

    REQUIRE_STREQ(time_s1.data(), time_s2.data());

    // Increment the timestamp for the next loop
    raw_ts += 1;
  }
}

/***/
TEST_CASE("string_from_time_gmtime_main_format")
{
  std::string fmt2 = "%Y-%m-%dT%H:%M:%SZ";
  StringFromTime string_from_time;

  string_from_time.init(fmt2, Timezone::GmtTime);

  // Get the timestamp now
  time_t raw_ts;
  std::time(&raw_ts);

  // Try for a few timestamps
  for (uint32_t i = 0; i <= 600'000; ++i)
  {
    // Get the time from string from time
    auto const& time_s1 = string_from_time.format_timestamp(raw_ts);

    // Get the time from strftime
    std::tm time_info{};
    quill::detail::gmtime_rs(&raw_ts, &time_info);
    char buffer[256];
    std::strftime(buffer, 256, fmt2.data(), &time_info);
    auto const time_s2 = std::string{buffer};

    REQUIRE_STREQ(time_s1.data(), time_s2.data());

    // Increment the timestamp for the next loop
    raw_ts += 1;
  }
}

TEST_CASE("string_from_time_localtime_main_format_increment_ts")
{
  std::string fmt2 = "%Y-%m-%dT%H:%M:%SZ";
  StringFromTime string_from_time;

  string_from_time.init(fmt2, Timezone::LocalTime);

  // Get the timestamp now
  time_t raw_ts;
  std::time(&raw_ts);

  // Try for a few timestamps
  for (uint32_t i = 0; i <= 10'000; ++i)
  {
    // Get the time from string from time
    auto const& time_s1 = string_from_time.format_timestamp(raw_ts);

    // Get the time from strftime
    std::tm time_info{};
    quill::detail::localtime_rs(&raw_ts, &time_info);
    char buffer[256];
    std::strftime(buffer, 256, fmt2.data(), &time_info);
    auto const time_s2 = std::string{buffer};

    REQUIRE_STREQ(time_s1.data(), time_s2.data());

    // Increment the timestamp for the next loop
    // This test increments the ts by a huge amount trying to mimic e.g. system clock changes
    raw_ts += 7200;
  }
}

/***/
TEST_CASE("string_from_time_localtime_empty_cached_indexes")
{
  // try with a format that doesn't have hours, minutes, seconds
  std::string fmt2 = "%Y-%m-%d";
  StringFromTime string_from_time;

  string_from_time.init(fmt2, Timezone::LocalTime);

  // Get the timestamp now
  time_t raw_ts;
  std::time(&raw_ts);

  // Try for a few timestamps
  for (uint32_t i = 0; i <= 500'000; ++i)
  {
    // Get the time from string from time
    auto const& time_s1 = string_from_time.format_timestamp(raw_ts);

    // Get the time from strftime
    std::tm time_info{};
    quill::detail::localtime_rs(&raw_ts, &time_info);
    char buffer[256];
    std::strftime(buffer, 256, fmt2.data(), &time_info);
    auto const time_s2 = std::string{buffer};

    REQUIRE_STREQ(time_s1.data(), time_s2.data());

    // Increment the timestamp for the next loop
    raw_ts += 1;
  }
}

/***/
TEST_CASE("string_from_time_localtime_format_l")
{
#if !defined(_WIN32)
  // The following tests don't run on windows because the format identifiers are not supported.
  std::string fmt2 = "%l:%M:%S%p";
  StringFromTime string_from_time;

  string_from_time.init(fmt2, Timezone::LocalTime);

  // Get the timestamp now
  time_t raw_ts;
  std::time(&raw_ts);

  // Try for a few timestamps
  for (uint32_t i = 0; i <= 500'000; ++i)
  {
    // Get the time from string from time
    auto const& time_s1 = string_from_time.format_timestamp(raw_ts);

    // Get the time from strftime
    std::tm* time_info;
    time_info = std::localtime(&raw_ts);
    char buffer[256];
    std::strftime(buffer, 256, fmt2.data(), time_info);
    auto const time_s2 = std::string{buffer};

    REQUIRE_STREQ(time_s1.data(), time_s2.data());

    // Increment the timestamp for the next loop
    raw_ts += 1;
  }
#else
  return;
#endif
}

/***/
TEST_CASE("string_from_time_localtime_format_k")
{
#if !defined(_WIN32)
  std::string fmt2 = "%k:%M:%S%p";
  StringFromTime string_from_time;

  string_from_time.init(fmt2, Timezone::LocalTime);

  // Get the timestamp now
  time_t raw_ts;
  std::time(&raw_ts);

  // Try for a few timestamps
  for (uint32_t i = 0; i <= 500'000; ++i)
  {
    // Get the time from string from time
    auto const& time_s1 = string_from_time.format_timestamp(raw_ts);

    // Get the time from strftime
    std::tm* time_info;
    time_info = std::localtime(&raw_ts);
    char buffer[256];
    std::strftime(buffer, 256, fmt2.data(), time_info);
    auto const time_s2 = std::string{buffer};

    REQUIRE_STREQ(time_s1.data(), time_s2.data());

    // Increment the timestamp for the next loop
    raw_ts += 1;
  }
#else
  return;
#endif
}

/***/
TEST_CASE("string_from_time_localtime_format_s")
{
#if !defined(_WIN32)
  std::string fmt2 = "%Y-%m-%d %s";
  StringFromTime string_from_time;

  string_from_time.init(fmt2, Timezone::LocalTime);

  // Get the timestamp now
  time_t raw_ts;
  std::time(&raw_ts);

  // Try for a few timestamps
  for (uint32_t i = 0; i <= 100'000; ++i)
  {
    // Get the time from string from time
    auto const& time_s1 = string_from_time.format_timestamp(raw_ts);

    // Get the time from strftime
    std::tm* time_info;
    time_info = std::localtime(&raw_ts);
    char buffer[256];
    std::strftime(buffer, 256, fmt2.data(), time_info);
    auto const time_s2 = std::string{buffer};

    REQUIRE_STREQ(time_s1.data(), time_s2.data());

    // Increment the timestamp for the next loop
    raw_ts += 1;
  }
#else
  return;
#endif
}

/***/
TEST_CASE("string_from_time_gmtime_format_s_crossing_10_digits_preserves_surrounding_literals")
{
#if !defined(_WIN32)
  std::string fmt = "X%sZ";
  StringFromTime string_from_time;
  string_from_time.init(fmt, Timezone::GmtTime);

  for (time_t raw_ts = 999999998; raw_ts <= 1000000001; ++raw_ts)
  {
    auto const& actual = string_from_time.format_timestamp(raw_ts);

    // %s is timezone-independent, so the expected value is the raw epoch value. Note that
    // strftime's %s cannot be used as a reference here: it round-trips through mktime() which
    // interprets the gmtime broken-down time as local time, producing a utc-offset-shifted
    // value on non-utc machines
    std::string const expected = "X" + std::to_string(raw_ts) + "Z";

    REQUIRE_STREQ(actual.data(), expected.data());
  }
#else
  return;
#endif
}

/***/
TEST_CASE("string_from_time_gmtime_format_s_crossing_11_digits_preserves_suffix")
{
#if !defined(_WIN32)
  if (sizeof(time_t) <= 4)
  {
    return;
  }

  std::string fmt = "%sZ";
  StringFromTime string_from_time;
  string_from_time.init(fmt, Timezone::GmtTime);

  for (time_t raw_ts = static_cast<time_t>(9999999998LL); raw_ts <= static_cast<time_t>(10000000001LL); ++raw_ts)
  {
    auto const& actual = string_from_time.format_timestamp(raw_ts);

    // %s is timezone-independent, so the expected value is the raw epoch value
    std::string const expected = std::to_string(raw_ts) + "Z";

    REQUIRE_STREQ(actual.data(), expected.data());
  }
#else
  return;
#endif
}

/***/
TEST_CASE("string_from_time_gmtime_format_s_non_utc_timezone")
{
#if !defined(_WIN32)
  // %s in GmtTime mode must produce the raw epoch value regardless of the machine's local
  // timezone. It previously went through strftime, whose %s round-trips the gmtime broken-down
  // time via mktime() and shifted the value by the utc offset
  // Use a POSIX TZ string so the test does not depend on zoneinfo data being installed.
  ScopedTimezone const scoped_timezone{"EST5"};

  {
    std::string fmt = "%s";
    StringFromTime string_from_time;
    string_from_time.init(fmt, Timezone::GmtTime);

    time_t const start_ts = 1751884800;

    // first iteration exercises the full recalculation path, the rest the incremental path
    for (time_t raw_ts = start_ts; raw_ts <= start_ts + 3; ++raw_ts)
    {
      auto const& actual = string_from_time.format_timestamp(raw_ts);
      std::string const expected = std::to_string(raw_ts);
      REQUIRE_STREQ(actual.data(), expected.data());
    }

    // also exercise the back-in-time strftime fallback path
    time_t const back_in_time_ts = start_ts - 100;
    auto const& fallback_actual = string_from_time.format_timestamp(back_in_time_ts);
    std::string const fallback_expected = std::to_string(back_in_time_ts);
    REQUIRE_STREQ(fallback_actual.data(), fallback_expected.data());
  }
#else
  return;
#endif
}

/***/
TEST_CASE("string_from_time_gmtime_escaped_percent_modifiers_match_strftime")
{
  char const* formats[] = {"%%H literal", "prefix %%M %%S", "%%%H:%M:%S"};

  for (char const* fmt : formats)
  {
    StringFromTime string_from_time;
    string_from_time.init(fmt, Timezone::GmtTime);

    for (time_t raw_ts = 1579825361; raw_ts <= 1579825365; ++raw_ts)
    {
      auto const& actual = string_from_time.format_timestamp(raw_ts);

      std::tm time_info{};
      quill::detail::gmtime_rs(&raw_ts, &time_info);
      char expected[256];
      std::strftime(expected, sizeof(expected), fmt, &time_info);

      REQUIRE_STREQ(actual.data(), expected);
    }
  }
}

/***/
TEST_CASE("string_from_time_gmtime_escaped_composite_modifiers_match_strftime")
{
  char const* formats[] = {"%%r literal", "prefix %%R %%T", "%%%T %%r", "literal %%X"};

  for (char const* fmt : formats)
  {
    StringFromTime string_from_time;
    string_from_time.init(fmt, Timezone::GmtTime);

    for (time_t raw_ts = 1579825361; raw_ts <= 1579825365; ++raw_ts)
    {
      auto const& actual = string_from_time.format_timestamp(raw_ts);

      std::tm time_info{};
      quill::detail::gmtime_rs(&raw_ts, &time_info);
      char expected[256];
      std::strftime(expected, sizeof(expected), fmt, &time_info);

      REQUIRE_STREQ(actual.data(), expected);
    }
  }
}

class StringFromTimeMock : public quill::detail::StringFromTime
{
public:
  static time_t next_noon_or_midnight_timestamp(time_t timestamp) noexcept
  {
    return quill::detail::StringFromTime::_next_noon_or_midnight_timestamp(timestamp);
  }

  static time_t nearest_quarter_hour_timestamp(time_t timestamp) noexcept
  {
    return quill::detail::StringFromTime::_nearest_quarter_hour_timestamp(timestamp);
  }

  static time_t next_quarter_hour_timestamp(time_t timestamp) noexcept
  {
    return quill::detail::StringFromTime::_next_quarter_hour_timestamp(timestamp);
  }

  static std::vector<char> safe_strftime(char const* format_string, time_t timestamp, Timezone timezone)
  {
    return quill::detail::StringFromTime::_safe_strftime(format_string, timestamp, timezone);
  }
};

/***/
TEST_CASE("next_noon_or_midnight_timestamp")
{
  // We do not test local because if we hardcode an expected_timestamp in our local timezone
  // it can be different in another system

  {
    // Noon utc
    time_t constexpr timestamp{1599033200};
    time_t constexpr expected_timestamp{1599048000};
    time_t const res = StringFromTimeMock::next_noon_or_midnight_timestamp(timestamp);
    REQUIRE_EQ(res, expected_timestamp);
  }

  {
    // Midnight utc
    time_t constexpr timestamp{1599079200};
    time_t constexpr expected_timestamp{1599091200};
    time_t const res = StringFromTimeMock::next_noon_or_midnight_timestamp(timestamp);
    REQUIRE_EQ(res, expected_timestamp);
  }
}

/***/
TEST_CASE("nearest_quarter_hour_timestamp")
{
  time_t constexpr timestamp = 1599473669;
  time_t constexpr expected_timestamp = 1599472800;
  REQUIRE_EQ(StringFromTimeMock::nearest_quarter_hour_timestamp(timestamp), expected_timestamp);
}

/***/
TEST_CASE("next_quarter_hour_timestamp")
{
  {
    time_t constexpr timestamp = 1599473669;
    time_t constexpr expected_timestamp = 1599473700;
    REQUIRE_EQ(StringFromTimeMock::next_quarter_hour_timestamp(timestamp), expected_timestamp);
  }

  {
    time_t constexpr timestamp = 1599473700;
    time_t constexpr expected_timestamp = 1599474600;
    REQUIRE_EQ(StringFromTimeMock::next_quarter_hour_timestamp(timestamp), expected_timestamp);
  }
}

/***/
TEST_CASE("safe_strftime_resize")
{
  // e.g. "Monday September 2020 (09/07/20) 15:37 EEST"
  constexpr char const* format_string = "%A %B %Y (%x) %H:%M %Z";

  // Get the timestamp now
  time_t raw_ts;
  std::time(&raw_ts);

  std::tm time_info{};
  quill::detail::localtime_rs(&raw_ts, &time_info);

  // we will format a string greater than 32
  char expected_result[256];
  std::strftime(expected_result, 256, format_string, &time_info);

  // Also try our version
  std::string const safe_strftime_result =
    std::string{StringFromTimeMock::safe_strftime(format_string, raw_ts, Timezone::LocalTime).data()};

  REQUIRE_STREQ(expected_result, safe_strftime_result.data());
}

TEST_CASE("safe_strftime_long_literal_resize")
{
  // Get the timestamp now
  time_t raw_ts;
  std::time(&raw_ts);

  std::string const format_string(200, 'x');

  std::string const safe_strftime_result =
    std::string{StringFromTimeMock::safe_strftime(format_string.data(), raw_ts, Timezone::LocalTime).data()};

  REQUIRE_EQ(format_string, safe_strftime_result);
}

TEST_CASE("safe_strftime_empty")
{
  // Get the timestamp now
  time_t raw_ts;
  std::time(&raw_ts);

  std::tm time_info{};
  quill::detail::localtime_rs(&raw_ts, &time_info);

  char expected_result[256];

#if !defined(__clang__) && defined(__GNUC__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wformat-zero-length"
#endif

  std::strftime(expected_result, 256, "", &time_info);

#if !defined(__clang__) && defined(__GNUC__)
  #pragma GCC diagnostic pop
#endif

  // Also try our version
  std::string const safe_strftime_result =
    std::string{StringFromTimeMock::safe_strftime("", raw_ts, Timezone::LocalTime).data()};

  REQUIRE_STREQ(expected_result, safe_strftime_result.data());
}

TEST_CASE("string_from_time_large_forward_jump_matches_strftime")
{
  std::string fmt = "%H:%M:%S";
  StringFromTime string_from_time;
  string_from_time.init(fmt, Timezone::GmtTime);

  time_t base_ts = 1;
  auto const& initial = string_from_time.format_timestamp(base_ts);
  (void)initial;

  time_t const jumped_ts = base_ts + static_cast<time_t>(std::numeric_limits<uint32_t>::max()) + 3601;
  auto const& actual = string_from_time.format_timestamp(jumped_ts);

  std::tm time_info{};
  quill::detail::gmtime_rs(&jumped_ts, &time_info);
  char expected[256];
  std::strftime(expected, sizeof(expected), fmt.data(), &time_info);

  REQUIRE_STREQ(actual.data(), expected);
}

TEST_SUITE_END();
