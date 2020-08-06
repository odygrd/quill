#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/LogLevel.h"
#include "quill/QuillError.h"

TEST_SUITE_BEGIN("LogLevel");

using namespace quill;
using namespace std::literals;

/***/
TEST_CASE("to_string")
{
  {
    LogLevel log_level{LogLevel::None};
    REQUIRE_STREQ(to_string(log_level), "None");
  }

  {
    LogLevel log_level{LogLevel::Backtrace};
    REQUIRE_STREQ(to_string(log_level), "Backtrace");
  }

  {
    LogLevel log_level{LogLevel::Critical};
    REQUIRE_STREQ(to_string(log_level), "Critical");
  }

  {
    LogLevel log_level{LogLevel::Error};
    REQUIRE_STREQ(to_string(log_level), "Error");
  }

  {
    LogLevel log_level{LogLevel::Warning};
    REQUIRE_STREQ(to_string(log_level), "Warning");
  }

  {
    LogLevel log_level{LogLevel::Info};
    REQUIRE_STREQ(to_string(log_level), "Info");
  }

  {
    LogLevel log_level{LogLevel::Debug};
    REQUIRE_STREQ(to_string(log_level), "Debug");
  }

  {
    LogLevel log_level{LogLevel::TraceL1};
    REQUIRE_STREQ(to_string(log_level), "TraceL1");
  }

  {
    LogLevel log_level{LogLevel::TraceL2};
    REQUIRE_STREQ(to_string(log_level), "TraceL2");
  }

  {
    LogLevel log_level{LogLevel::TraceL3};
    REQUIRE_STREQ(to_string(log_level), "TraceL3");
  }

  {
#ifndef QUILL_NO_EXCEPTIONS
    LogLevel log_level;
    log_level = static_cast<LogLevel>(-1);
    REQUIRE_THROWS_AS(QUILL_MAYBE_UNUSED auto s = to_string(log_level), quill::QuillError);
#endif
  }
}

/***/
TEST_CASE("from_string")
{
  {
    std::string log_level{"None"};
    REQUIRE_EQ(from_string(log_level), LogLevel::None);
  }

  {
    std::string log_level{"Backtrace"};
    REQUIRE_EQ(from_string(log_level), LogLevel::Backtrace);
  }

  {
    std::string log_level{"Critical"};
    REQUIRE_EQ(from_string(log_level), LogLevel::Critical);
  }

  {
    std::string log_level{"Error"};
    REQUIRE_EQ(from_string(log_level), LogLevel::Error);
  }

  {
    std::string log_level{"Warning"};
    REQUIRE_EQ(from_string(log_level), LogLevel::Warning);
  }

  {
    std::string log_level{"Info"};
    REQUIRE_EQ(from_string(log_level), LogLevel::Info);
  }

  {
    std::string log_level{"Debug"};
    REQUIRE_EQ(from_string(log_level), LogLevel::Debug);
  }

  {
    std::string log_level{"TraceL1"};
    REQUIRE_EQ(from_string(log_level), LogLevel::TraceL1);
  }

  {
    std::string log_level{"TraceL2"};
    REQUIRE_EQ(from_string(log_level), LogLevel::TraceL2);
  }

  {
    std::string log_level{"TraceL3"};
    REQUIRE_EQ(from_string(log_level), LogLevel::TraceL3);
  }

  {
#ifndef QUILL_NO_EXCEPTIONS
    std::string log_level{"dummy"};
    REQUIRE_THROWS_AS(QUILL_MAYBE_UNUSED auto res = from_string(log_level), quill::QuillError);
#endif
  }
}

TEST_SUITE_END();