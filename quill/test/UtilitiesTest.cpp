#include "doctest/doctest.h"

#include "DocTestExtensions.h"
#include "quill/detail/misc/Os.h"
#include "quill/detail/misc/Utilities.h"
#include <chrono>
#include <ctime>
#include <thread>

using namespace quill;
using namespace quill::detail;

TEST_SUITE_BEGIN("Utilities");

/***/
TEST_CASE("next_noon_or_midnight_timestamp")
{
  // We do not test local because if we hardcode an expected_timestamp in our local timezone
  // it can be different in another system
  
  {
    // Noon utc
    time_t timestamp{1599033200};
    time_t expected_timestamp{1599048000};
    time_t const res = next_noon_or_midnight_timestamp(timestamp, Timezone::GmtTime);
    REQUIRE_EQ(res, expected_timestamp);
  }

  {
    // Midnight utc
    time_t timestamp{1599079200};
    time_t expected_timestamp{1599091200};
    time_t const res = next_noon_or_midnight_timestamp(timestamp, Timezone::GmtTime);
    REQUIRE_EQ(res, expected_timestamp);
  }
}

/***/
TEST_CASE("nearest_hour_timestamp")
{
  time_t const timestamp = 1599473669;
  time_t const expected_timestamp = 1599472800;
  REQUIRE_EQ(nearest_hour_timestamp(timestamp), expected_timestamp);
}

/***/
TEST_CASE("next_hour_timestamp")
{
  time_t const timestamp = 1599473669;
  time_t const expected_timestamp = 1599476400;
  REQUIRE_EQ(next_hour_timestamp(timestamp), expected_timestamp);
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
    std::string{safe_strftime(format_string, raw_ts, Timezone::LocalTime).data()};

  REQUIRE_STREQ(expected_result, safe_strftime_result.data());
}

TEST_CASE("safe_strftime_empty")
{
  // Get the timestamp now
  time_t raw_ts;
  std::time(&raw_ts);

  std::tm time_info{};
  quill::detail::localtime_rs(&raw_ts, &time_info);

  // we will format a string greater than 32
  char expected_result[256];
  std::strftime(expected_result, 256, "", &time_info);

  // Also try our version
  std::string const safe_strftime_result =
    std::string{safe_strftime("", raw_ts, Timezone::LocalTime).data()};

  REQUIRE_STREQ(expected_result, safe_strftime_result.data());
}

TEST_CASE("set_get_thread_name")
{
  std::thread t1{[](){
#ifdef QUILL_NO_THREAD_NAME_SUPPORT
    std::string const tname{};
#else
    std::string const tname{"test_thread"};
#endif
    set_thread_name(tname.data());
    std::string const res = get_thread_name();
    REQUIRE_EQ(tname, res);
  }};

  t1.join();
}
TEST_SUITE_END();
