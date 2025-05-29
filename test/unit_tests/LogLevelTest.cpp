#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/backend/BackendOptions.h"
#include "quill/core/LogLevel.h"
#include "quill/core/QuillError.h"

#include <array>

TEST_SUITE_BEGIN("LogLevel");

using namespace quill;
using namespace std::literals;

/***/
TEST_CASE("loglevel_to_string")
{
  BackendOptions bo;

  REQUIRE_EQ(bo.log_level_short_codes.size(), bo.log_level_descriptions.size());

  {
    LogLevel log_level{LogLevel::None};
    REQUIRE_STREQ(detail::log_level_to_string(log_level, bo.log_level_descriptions.data(),
                                              bo.log_level_descriptions.size())
                    .data(),
                  "NONE");
  }

  {
    LogLevel log_level{LogLevel::Backtrace};
    REQUIRE_STREQ(detail::log_level_to_string(log_level, bo.log_level_descriptions.data(),
                                              bo.log_level_descriptions.size())
                    .data(),
                  "BACKTRACE");
  }

  {
    LogLevel log_level{LogLevel::Critical};
    REQUIRE_STREQ(detail::log_level_to_string(log_level, bo.log_level_descriptions.data(),
                                              bo.log_level_descriptions.size())
                    .data(),
                  "CRITICAL");
  }

  {
    LogLevel log_level{LogLevel::Error};
    REQUIRE_STREQ(detail::log_level_to_string(log_level, bo.log_level_descriptions.data(),
                                              bo.log_level_descriptions.size())
                    .data(),
                  "ERROR");
  }

  {
    LogLevel log_level{LogLevel::Warning};
    REQUIRE_STREQ(detail::log_level_to_string(log_level, bo.log_level_descriptions.data(),
                                              bo.log_level_descriptions.size())
                    .data(),
                  "WARNING");
  }

  {
    LogLevel log_level{LogLevel::Notice};
    REQUIRE_STREQ(detail::log_level_to_string(log_level, bo.log_level_descriptions.data(),
                                              bo.log_level_descriptions.size())
                    .data(),
                  "NOTICE");
  }

  {
    LogLevel log_level{LogLevel::Info};
    REQUIRE_STREQ(detail::log_level_to_string(log_level, bo.log_level_descriptions.data(),
                                              bo.log_level_descriptions.size())
                    .data(),
                  "INFO");
  }

  {
    LogLevel log_level{LogLevel::Debug};
    REQUIRE_STREQ(detail::log_level_to_string(log_level, bo.log_level_descriptions.data(),
                                              bo.log_level_descriptions.size())
                    .data(),
                  "DEBUG");
  }

  {
    LogLevel log_level{LogLevel::TraceL1};
    REQUIRE_STREQ(detail::log_level_to_string(log_level, bo.log_level_descriptions.data(),
                                              bo.log_level_descriptions.size())
                    .data(),
                  "TRACE_L1");
  }

  {
    LogLevel log_level{LogLevel::TraceL2};
    REQUIRE_STREQ(detail::log_level_to_string(log_level, bo.log_level_descriptions.data(),
                                              bo.log_level_descriptions.size())
                    .data(),
                  "TRACE_L2");
  }

  {
    LogLevel log_level{LogLevel::TraceL3};
    REQUIRE_STREQ(detail::log_level_to_string(log_level, bo.log_level_descriptions.data(),
                                              bo.log_level_descriptions.size())
                    .data(),
                  "TRACE_L3");
  }

  {
#ifndef QUILL_NO_EXCEPTIONS
    LogLevel log_level;
    log_level = static_cast<LogLevel>(100);
    auto func = [log_level, bo]()
    {
      auto s = detail::log_level_to_string(log_level, bo.log_level_descriptions.data(),
                                           bo.log_level_descriptions.size())
                 .data();
      return s;
    };

    REQUIRE_THROWS_AS(func(), quill::QuillError);
#endif
  }
}

/***/
TEST_CASE("loglevel_to_short_code")
{
  BackendOptions bo;

  {
    LogLevel log_level{LogLevel::None};
    REQUIRE_STREQ(detail::log_level_to_string(log_level, bo.log_level_short_codes.data(),
                                              bo.log_level_short_codes.size())
                    .data(),
                  "_");
  }

  {
    LogLevel log_level{LogLevel::Backtrace};
    REQUIRE_STREQ(detail::log_level_to_string(log_level, bo.log_level_short_codes.data(),
                                              bo.log_level_short_codes.size())
                    .data(),
                  "BT");
  }

  {
    LogLevel log_level{LogLevel::Critical};
    REQUIRE_STREQ(detail::log_level_to_string(log_level, bo.log_level_short_codes.data(),
                                              bo.log_level_short_codes.size())
                    .data(),
                  "C");
  }

  {
    LogLevel log_level{LogLevel::Error};
    REQUIRE_STREQ(detail::log_level_to_string(log_level, bo.log_level_short_codes.data(),
                                              bo.log_level_short_codes.size())
                    .data(),
                  "E");
  }

  {
    LogLevel log_level{LogLevel::Warning};
    REQUIRE_STREQ(detail::log_level_to_string(log_level, bo.log_level_short_codes.data(),
                                              bo.log_level_short_codes.size())
                    .data(),
                  "W");
  }

  {
    LogLevel log_level{LogLevel::Notice};
    REQUIRE_STREQ(detail::log_level_to_string(log_level, bo.log_level_short_codes.data(),
                                              bo.log_level_short_codes.size())
                    .data(),
                  "N");
  }

  {
    LogLevel log_level{LogLevel::Info};
    REQUIRE_STREQ(detail::log_level_to_string(log_level, bo.log_level_short_codes.data(),
                                              bo.log_level_short_codes.size())
                    .data(),
                  "I");
  }

  {
    LogLevel log_level{LogLevel::Debug};
    REQUIRE_STREQ(detail::log_level_to_string(log_level, bo.log_level_short_codes.data(),
                                              bo.log_level_short_codes.size())
                    .data(),
                  "D");
  }

  {
    LogLevel log_level{LogLevel::TraceL1};
    REQUIRE_STREQ(detail::log_level_to_string(log_level, bo.log_level_short_codes.data(),
                                              bo.log_level_short_codes.size())
                    .data(),
                  "T1");
  }

  {
    LogLevel log_level{LogLevel::TraceL2};
    REQUIRE_STREQ(detail::log_level_to_string(log_level, bo.log_level_short_codes.data(),
                                              bo.log_level_short_codes.size())
                    .data(),
                  "T2");
  }

  {
    LogLevel log_level{LogLevel::TraceL3};
    REQUIRE_STREQ(detail::log_level_to_string(log_level, bo.log_level_short_codes.data(),
                                              bo.log_level_short_codes.size())
                    .data(),
                  "T3");
  }

  {
#ifndef QUILL_NO_EXCEPTIONS
    LogLevel log_level;
    log_level = static_cast<LogLevel>(100);
    auto func = [log_level, bo]()
    {
      auto s = detail::log_level_to_string(log_level, bo.log_level_short_codes.data(),
                                           bo.log_level_short_codes.size())
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
    std::string log_level{"Trace_L1"};
    REQUIRE_EQ(loglevel_from_string(log_level), LogLevel::TraceL1);
  }

  {
    std::string log_level{"Trace_L2"};
    REQUIRE_EQ(loglevel_from_string(log_level), LogLevel::TraceL2);
  }

  {
    std::string log_level{"Trace_L3"};
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