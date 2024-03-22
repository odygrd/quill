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
    constexpr MacroMetadata log_line_info{__FILE__ ":" QUILL_STRINGIFY(__LINE__),
                                          __FUNCTION__,
                                          "Test fmt {}",
                                          nullptr,
                                          quill::LogLevel::Debug,
                                          MacroMetadata::Event::Log,
                                          true,
                                          false};

    REQUIRE_STREQ(log_line_info.message_format(), "Test fmt {}");
    REQUIRE_EQ(log_line_info.log_level(), quill::LogLevel::Debug);
    REQUIRE_STREQ(log_line_info.line(), "15");
  }

  {
    constexpr MacroMetadata log_line_info{__FILE__ ":" QUILL_STRINGIFY(__LINE__),
                                          __FUNCTION__,
                                          "Test another fmt {}",
                                          nullptr,
                                          quill::LogLevel::Info,
                                          MacroMetadata::Event::Log,
                                          true,
                                          false};

    REQUIRE_STREQ(log_line_info.message_format(), "Test another fmt {}");
    REQUIRE_EQ(log_line_info.log_level(), quill::LogLevel::Info);
    REQUIRE_STREQ(log_line_info.line(), "30");
    REQUIRE_EQ(log_line_info.file_name(), std::string_view {"MacroMetadataTest.cpp"});
    REQUIRE_STREQ(log_line_info.short_source_location(), "MacroMetadataTest.cpp:30");
  }
}

TEST_SUITE_END();