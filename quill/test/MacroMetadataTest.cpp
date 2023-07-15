#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/MacroMetadata.h"
#include "quill/detail/misc/Common.h"

TEST_SUITE_BEGIN("MacroMetadata");

using namespace quill::detail;
using namespace quill;

TEST_CASE("construct")
{
  {
    constexpr MacroMetadata log_line_info{QUILL_STRINGIFY(__LINE__),
                                          __FILE__,
                                          "",
                                          __FUNCTION__,
                                          "Test fmt {}",
                                          quill::LogLevel::Debug,
                                          MacroMetadata::Event::Log,
                                          true,
                                          false};

    REQUIRE_STREQ(log_line_info.message_format().data(), "Test fmt {}");
    REQUIRE_EQ(log_line_info.level(), quill::LogLevel::Debug);
    REQUIRE_STREQ(log_line_info.lineno().data(), "15");
  }

  {
    constexpr MacroMetadata log_line_info{QUILL_STRINGIFY(__LINE__),
                                          __FILE__,
                                          "",
                                          __FUNCTION__,
                                          "Test another fmt {}",
                                          quill::LogLevel::Info,
                                          MacroMetadata::Event::Log,
                                          true,
                                          false};

    REQUIRE_STREQ(log_line_info.message_format().data(), "Test another fmt {}");
    REQUIRE_EQ(log_line_info.level(), quill::LogLevel::Info);
    REQUIRE_STREQ(log_line_info.lineno().data(), "31");
    REQUIRE_STREQ(log_line_info.filename().data(), "MacroMetadataTest.cpp");
  }
}

TEST_SUITE_END();