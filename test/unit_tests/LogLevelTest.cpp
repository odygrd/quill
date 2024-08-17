#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/core/LogLevel.h"
#include "quill/core/QuillError.h"

#include <array>

TEST_SUITE_BEGIN("LogLevel");

using namespace quill;
using namespace std::literals;

/***/
TEST_CASE("loglevel_to_string")
{
  std::array<std::string, 12> log_level_descriptions = {
    "TRACE_L3", "TRACE_L2", "TRACE_L1",  "DEBUG", "INFO", "NOTICE", "WARNING",
    "ERROR",    "CRITICAL", "BACKTRACE", "NONE",  "DYNAMIC"};

  {
    LogLevel log_level{LogLevel::Dynamic};
    REQUIRE_STREQ(
      detail::log_level_to_string(log_level, log_level_descriptions.data(), log_level_descriptions.size())
        .data(),
      "DYNAMIC");
  }

  {
    LogLevel log_level{LogLevel::None};
    REQUIRE_STREQ(
      detail::log_level_to_string(log_level, log_level_descriptions.data(), log_level_descriptions.size())
        .data(),
      "NONE");
  }

  {
    LogLevel log_level{LogLevel::Backtrace};
    REQUIRE_STREQ(
      detail::log_level_to_string(log_level, log_level_descriptions.data(), log_level_descriptions.size())
        .data(),
      "BACKTRACE");
  }

  {
    LogLevel log_level{LogLevel::Critical};
    REQUIRE_STREQ(
      detail::log_level_to_string(log_level, log_level_descriptions.data(), log_level_descriptions.size())
        .data(),
      "CRITICAL");
  }

  {
    LogLevel log_level{LogLevel::Error};
    REQUIRE_STREQ(
      detail::log_level_to_string(log_level, log_level_descriptions.data(), log_level_descriptions.size())
        .data(),
      "ERROR");
  }

  {
    LogLevel log_level{LogLevel::Warning};
    REQUIRE_STREQ(
      detail::log_level_to_string(log_level, log_level_descriptions.data(), log_level_descriptions.size())
        .data(),
      "WARNING");
  }

  {
    LogLevel log_level{LogLevel::Notice};
    REQUIRE_STREQ(
      detail::log_level_to_string(log_level, log_level_descriptions.data(), log_level_descriptions.size())
        .data(),
      "NOTICE");
  }

  {
    LogLevel log_level{LogLevel::Info};
    REQUIRE_STREQ(
      detail::log_level_to_string(log_level, log_level_descriptions.data(), log_level_descriptions.size())
        .data(),
      "INFO");
  }

  {
    LogLevel log_level{LogLevel::Debug};
    REQUIRE_STREQ(
      detail::log_level_to_string(log_level, log_level_descriptions.data(), log_level_descriptions.size())
        .data(),
      "DEBUG");
  }

  {
    LogLevel log_level{LogLevel::TraceL1};
    REQUIRE_STREQ(
      detail::log_level_to_string(log_level, log_level_descriptions.data(), log_level_descriptions.size())
        .data(),
      "TRACE_L1");
  }

  {
    LogLevel log_level{LogLevel::TraceL2};
    REQUIRE_STREQ(
      detail::log_level_to_string(log_level, log_level_descriptions.data(), log_level_descriptions.size())
        .data(),
      "TRACE_L2");
  }

  {
    LogLevel log_level{LogLevel::TraceL3};
    REQUIRE_STREQ(
      detail::log_level_to_string(log_level, log_level_descriptions.data(), log_level_descriptions.size())
        .data(),
      "TRACE_L3");
  }

  {
#ifndef QUILL_NO_EXCEPTIONS
    LogLevel log_level;
    log_level = static_cast<LogLevel>(100);
    auto func = [log_level, log_level_descriptions]()
    {
      auto s = detail::log_level_to_string(log_level, log_level_descriptions.data(),
                                           log_level_descriptions.size())
                 .data();
      return s;
    };

    REQUIRE_THROWS_AS(func(), quill::QuillError);
#endif
  }
}

/***/
TEST_CASE("loglevel_from_string")
{
  {
    std::string log_level{"Dynamic"};
    REQUIRE_EQ(loglevel_from_string(log_level), LogLevel::Dynamic);
  }

  {
    std::string log_level{"None"};
    REQUIRE_EQ(loglevel_from_string(log_level), LogLevel::None);
  }

  {
    std::string log_level{"Backtrace"};
    REQUIRE_EQ(loglevel_from_string(log_level), LogLevel::Backtrace);
  }

  {
    std::string log_level{"Critical"};
    REQUIRE_EQ(loglevel_from_string(log_level), LogLevel::Critical);
  }

  {
    std::string log_level{"Error"};
    REQUIRE_EQ(loglevel_from_string(log_level), LogLevel::Error);
  }

  {
    std::string log_level{"Warning"};
    REQUIRE_EQ(loglevel_from_string(log_level), LogLevel::Warning);
  }

  {
    std::string log_level{"Notice"};
    REQUIRE_EQ(loglevel_from_string(log_level), LogLevel::Notice);
  }

  {
    std::string log_level{"Info"};
    REQUIRE_EQ(loglevel_from_string(log_level), LogLevel::Info);
  }

  {
    std::string log_level{"Debug"};
    REQUIRE_EQ(loglevel_from_string(log_level), LogLevel::Debug);
  }

  {
    std::string log_level{"TraceL1"};
    REQUIRE_EQ(loglevel_from_string(log_level), LogLevel::TraceL1);
  }

  {
    std::string log_level{"TraceL2"};
    REQUIRE_EQ(loglevel_from_string(log_level), LogLevel::TraceL2);
  }

  {
    std::string log_level{"TraceL3"};
    REQUIRE_EQ(loglevel_from_string(log_level), LogLevel::TraceL3);
  }

  {
#ifndef QUILL_NO_EXCEPTIONS
    std::string log_level{"dummy"};
    auto func = [log_level]()
    {
      auto res = loglevel_from_string(log_level);
      return res;
    };
    REQUIRE_THROWS_AS(func(), quill::QuillError);
#endif
  }
}

TEST_SUITE_END();