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
                                           MacroMetadata::Event::Log};

    REQUIRE_STREQ(macro_metadata.message_format(), "Test fmt {}");
    REQUIRE_EQ(macro_metadata.log_level(), quill::LogLevel::Debug);
    REQUIRE_STREQ(macro_metadata.line(), "15");
    REQUIRE_EQ(macro_metadata.has_named_args(), false);
  }

  {
    constexpr MacroMetadata macro_metadata{__FILE__ ":" QUILL_STRINGIFY(__LINE__),
                                           __FUNCTION__,
                                           "Test another fmt {name}",
                                           nullptr,
                                           quill::LogLevel::Info,
                                           MacroMetadata::Event::Flush};

    REQUIRE_STREQ(macro_metadata.message_format(), "Test another fmt {name}");
    REQUIRE_EQ(macro_metadata.log_level(), quill::LogLevel::Info);
    REQUIRE_STREQ(macro_metadata.line(), "29");
    REQUIRE_EQ(macro_metadata.file_name(), std::string_view{"MacroMetadataTest.cpp"});
    REQUIRE_STREQ(macro_metadata.short_source_location(), "MacroMetadataTest.cpp:29");
    REQUIRE_STREQ(macro_metadata.caller_function(), "DOCTEST_ANON_FUNC_3");
    REQUIRE_EQ(macro_metadata.event(), MacroMetadata::Event::Flush);
    REQUIRE_EQ(macro_metadata.has_named_args(), true);
    REQUIRE_NE(std::string_view{macro_metadata.source_location()}.find("MacroMetadataTest.cpp"),
               std::string_view::npos);
    REQUIRE_NE(std::string_view{macro_metadata.full_path()}.find("MacroMetadataTest.cpp"),
               std::string_view::npos);
  }

  {
    constexpr MacroMetadata macro_metadata{__FILE__ ":" QUILL_STRINGIFY(__LINE__),
                                           __FUNCTION__,
                                           "Test another fmt {name} and {surname} and {{age}}",
                                           nullptr,
                                           quill::LogLevel::Info,
                                           MacroMetadata::Event::Flush};

    REQUIRE_STREQ(macro_metadata.message_format(),
                  "Test another fmt {name} and {surname} and {{age}}");
    REQUIRE_EQ(macro_metadata.log_level(), quill::LogLevel::Info);
    REQUIRE_STREQ(macro_metadata.line(), "51");
    REQUIRE_EQ(macro_metadata.file_name(), std::string_view{"MacroMetadataTest.cpp"});
    REQUIRE_STREQ(macro_metadata.short_source_location(), "MacroMetadataTest.cpp:51");
    REQUIRE_STREQ(macro_metadata.caller_function(), "DOCTEST_ANON_FUNC_3");
    REQUIRE_EQ(macro_metadata.event(), MacroMetadata::Event::Flush);
    REQUIRE_EQ(macro_metadata.has_named_args(), true);
  }

  {
    constexpr MacroMetadata macro_metadata{__FILE__ ":" QUILL_STRINGIFY(__LINE__),
                                           __FUNCTION__,
                                           "Test another fmt {0} and {1} and {2}",
                                           nullptr,
                                           quill::LogLevel::Info,
                                           MacroMetadata::Event::Flush};

    REQUIRE_STREQ(macro_metadata.message_format(), "Test another fmt {0} and {1} and {2}");
    REQUIRE_EQ(macro_metadata.log_level(), quill::LogLevel::Info);
    REQUIRE_STREQ(macro_metadata.line(), "70");
    REQUIRE_EQ(macro_metadata.file_name(), std::string_view{"MacroMetadataTest.cpp"});
    REQUIRE_STREQ(macro_metadata.short_source_location(), "MacroMetadataTest.cpp:70");
    REQUIRE_STREQ(macro_metadata.caller_function(), "DOCTEST_ANON_FUNC_3");
    REQUIRE_EQ(macro_metadata.event(), MacroMetadata::Event::Flush);
    REQUIRE_EQ(macro_metadata.has_named_args(), false);
  }
}

TEST_SUITE_END();