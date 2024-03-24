#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/core/Common.h"
#include "quill/core/MacroMetadata.h"

TEST_SUITE_BEGIN("MacroMetadata");

using namespace quill::detail;
using namespace quill;

TEST_CASE("construct")
{
  {
    constexpr MacroMetadata macro_metadata{__FILE__ ":" QUILL_STRINGIFY(__LINE__),
                                           __FUNCTION__,
                                           "Test fmt {}",
                                           nullptr,
                                           quill::LogLevel::Debug,
                                           MacroMetadata::Event::Log,
                                           false};

    REQUIRE_STREQ(macro_metadata.message_format(), "Test fmt {}");
    REQUIRE_EQ(macro_metadata.log_level(), quill::LogLevel::Debug);
    REQUIRE_STREQ(macro_metadata.line(), "15");
    REQUIRE_EQ(macro_metadata.is_structured_log_template(), false);
  }

  {
    constexpr MacroMetadata macro_metadata{__FILE__ ":" QUILL_STRINGIFY(__LINE__),
                                           __FUNCTION__,
                                           "Test another fmt {}",
                                           nullptr,
                                           quill::LogLevel::Info,
                                           MacroMetadata::Event::Flush,
                                           true};

    REQUIRE_STREQ(macro_metadata.message_format(), "Test another fmt {}");
    REQUIRE_EQ(macro_metadata.log_level(), quill::LogLevel::Info);
    REQUIRE_STREQ(macro_metadata.line(), "30");
    REQUIRE_EQ(macro_metadata.file_name(), std::string_view{"MacroMetadataTest.cpp"});
    REQUIRE_STREQ(macro_metadata.short_source_location(), "MacroMetadataTest.cpp:30");
    REQUIRE_STREQ(macro_metadata.caller_function(), "DOCTEST_ANON_FUNC_3");
    REQUIRE_EQ(macro_metadata.event(), MacroMetadata::Event::Flush);
    REQUIRE_EQ(macro_metadata.is_structured_log_template(), true);
  }
}

TEST_SUITE_END();