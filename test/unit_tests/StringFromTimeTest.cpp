#include "quill/backend/StringFromTime.h"
#include "DocTestExtensions.h"
#include "doctest/doctest.h"
#include <ctime>

TEST_SUITE_BEGIN("StringFromTime");

using namespace quill;
using namespace quill::detail;

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

#if !defined(_WIN32)
// The following tests don't run on windows because the format identifiers are not supported.

/***/
TEST_CASE("string_from_time_localtime_format_l")
{
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
}

/***/
TEST_CASE("string_from_time_localtime_format_k")
{
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
}

/***/
TEST_CASE("string_from_time_localtime_format_s")
{
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
}
#endif

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
  constexpr char const* format_string = "%A %B %Y (%x) %R %Z";

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

TEST_CASE("safe_strftime_empty")
{
  // Get the timestamp now
  time_t raw_ts;
  std::time(&raw_ts);

  std::tm time_info{};
  quill::detail::localtime_rs(&raw_ts, &time_info);

  char expected_result[256];

#if !defined(_WIN32)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wformat-zero-length"
#endif

  std::strftime(expected_result, 256, "", &time_info);
  
#if !defined(_WIN32)
  #pragma GCC diagnostic pop
#endif

  // Also try our version
  std::string const safe_strftime_result =
    std::string{StringFromTimeMock::safe_strftime("", raw_ts, Timezone::LocalTime).data()};

  REQUIRE_STREQ(expected_result, safe_strftime_result.data());
}

TEST_SUITE_END();