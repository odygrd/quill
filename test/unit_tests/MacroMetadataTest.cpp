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

  // Test block for a case with no named arguments (escaped braces only)
  {
    constexpr MacroMetadata macro_metadata{
      "src/NoNamed.cpp:100",               // source_location
      "NoNamedFunc",                       // caller_function
      "Test with no named args {{value}}", // message_format (escaped, so no named arg)
      "unit_test_tag",                     // tags
      quill::LogLevel::Debug,              // log_level
      MacroMetadata::Event::Log            // event
    };

    REQUIRE_STREQ(macro_metadata.message_format(), "Test with no named args {{value}}");
    REQUIRE_EQ(macro_metadata.log_level(), quill::LogLevel::Debug);
    REQUIRE_STREQ(macro_metadata.line(), "100");
    REQUIRE_EQ(macro_metadata.full_path(), std::string_view{"src/NoNamed.cpp"});
    REQUIRE_EQ(macro_metadata.file_name(), std::string_view{"NoNamed.cpp"});
    REQUIRE_STREQ(macro_metadata.short_source_location(), "NoNamed.cpp:100");
    REQUIRE_STREQ(macro_metadata.caller_function(), "NoNamedFunc");
    REQUIRE_EQ(macro_metadata.event(), MacroMetadata::Event::Log);
    REQUIRE_EQ(macro_metadata.has_named_args(), false);
  }

  // Test block for a case with multiple named arguments
  {
    constexpr MacroMetadata macro_metadata{
      "MacroMetadataTest.cpp:51",                          // source_location
      "DOCTEST_ANON_FUNC_3",                               // caller_function
      "Test another fmt {name} and {surname} and {{age}}", // message_format with named args {name} and {surname}
      nullptr,                                             // tags
      quill::LogLevel::Info,                               // log_level
      MacroMetadata::Event::Flush                          // event
    };

    REQUIRE_STREQ(macro_metadata.message_format(),
                  "Test another fmt {name} and {surname} and {{age}}");
    REQUIRE_EQ(macro_metadata.log_level(), quill::LogLevel::Info);
    REQUIRE_STREQ(macro_metadata.line(), "51");
    REQUIRE_EQ(macro_metadata.full_path(), std::string_view{"MacroMetadataTest.cpp"});
    REQUIRE_EQ(macro_metadata.file_name(), std::string_view{"MacroMetadataTest.cpp"});
    REQUIRE_STREQ(macro_metadata.short_source_location(), "MacroMetadataTest.cpp:51");
    REQUIRE_STREQ(macro_metadata.caller_function(), "DOCTEST_ANON_FUNC_3");
    REQUIRE_EQ(macro_metadata.event(), MacroMetadata::Event::Flush);
    REQUIRE_EQ(macro_metadata.has_named_args(), true);
  }

  // Test block for a case with empty braces (which should not count as named args)
  {
    constexpr MacroMetadata macro_metadata{
      "folder/EmptyBraces.cpp:77",                   // source_location
      "EmptyArgsFunc",                               // caller_function
      "Curly braces with no name: {} and also {{}}", // message_format with empty {}
      "tag_empty",                                   // tags
      quill::LogLevel::Warning,                      // log_level
      MacroMetadata::Event::FlushBacktrace           // event
    };

    REQUIRE_STREQ(macro_metadata.message_format(), "Curly braces with no name: {} and also {{}}");
    REQUIRE_EQ(macro_metadata.log_level(), quill::LogLevel::Warning);
    REQUIRE_STREQ(macro_metadata.line(), "77");
    REQUIRE_EQ(macro_metadata.full_path(), std::string_view{"folder/EmptyBraces.cpp"});
    REQUIRE_EQ(macro_metadata.file_name(), std::string_view{"EmptyBraces.cpp"});
    REQUIRE_STREQ(macro_metadata.short_source_location(), "EmptyBraces.cpp:77");
    REQUIRE_STREQ(macro_metadata.caller_function(), "EmptyArgsFunc");
    REQUIRE_EQ(macro_metadata.event(), MacroMetadata::Event::FlushBacktrace);
    // Since {} does not contain a valid letter and {{}} is escaped, no named arg should be detected.
    REQUIRE_EQ(macro_metadata.has_named_args(), false);
  }

  // Test block for a case with a single valid named argument
  {
    constexpr MacroMetadata macro_metadata{
      "lib/SingleArg.cpp:300", // source_location
      "SingleArgFunc",         // caller_function
      "Value is {x}",          // message_format with a single named argument {x}
      "tag_single",            // tags
      quill::LogLevel::Error,  // log_level
      MacroMetadata::Event::LogWithRuntimeMetadataDeepCopy // event
    };

    REQUIRE_STREQ(macro_metadata.message_format(), "Value is {x}");
    REQUIRE_EQ(macro_metadata.log_level(), quill::LogLevel::Error);
    REQUIRE_STREQ(macro_metadata.line(), "300");
    REQUIRE_EQ(macro_metadata.full_path(), std::string_view{"lib/SingleArg.cpp"});
    REQUIRE_EQ(macro_metadata.file_name(), std::string_view{"SingleArg.cpp"});
    REQUIRE_STREQ(macro_metadata.short_source_location(), "SingleArg.cpp:300");
    REQUIRE_STREQ(macro_metadata.caller_function(), "SingleArgFunc");
    REQUIRE_EQ(macro_metadata.event(), MacroMetadata::Event::LogWithRuntimeMetadataDeepCopy);
    REQUIRE_EQ(macro_metadata.has_named_args(), true);
  }

  // Test block for an edge case with a deep path
  {
    constexpr MacroMetadata macro_metadata{
      "a/b/c/directory/DeepFile.cpp:999", // source_location with a deep path
      "DeepFunc",                         // caller_function
      "No named args here",               // message_format with no named arguments
      "deep_tag",                         // tags
      quill::LogLevel::Info,              // log_level
      MacroMetadata::Event::InitBacktrace // event
    };

    REQUIRE_STREQ(macro_metadata.message_format(), "No named args here");
    REQUIRE_EQ(macro_metadata.log_level(), quill::LogLevel::Info);
    REQUIRE_STREQ(macro_metadata.line(), "999");
    REQUIRE_EQ(macro_metadata.full_path(), std::string_view{"a/b/c/directory/DeepFile.cpp"});
    REQUIRE_EQ(macro_metadata.file_name(), std::string_view{"DeepFile.cpp"});
    REQUIRE_STREQ(macro_metadata.short_source_location(), "DeepFile.cpp:999");
    REQUIRE_STREQ(macro_metadata.caller_function(), "DeepFunc");
    REQUIRE_EQ(macro_metadata.event(), MacroMetadata::Event::InitBacktrace);
    REQUIRE_EQ(macro_metadata.has_named_args(), false);
  }
}

TEST_SUITE_END();