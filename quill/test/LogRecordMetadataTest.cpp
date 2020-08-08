#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/detail/events/LogRecordMetadata.h"
#include "quill/detail/misc/Macros.h"

TEST_SUITE_BEGIN("LogRecordMetadata");

using namespace quill::detail;
using namespace quill;

TEST_CASE("construct")
{
  {
    constexpr LogRecordMetadata log_line_info{QUILL_STRINGIFY(__LINE__), __FILE__, __FUNCTION__,
                                              "Test fmt {}", quill::LogLevel::Debug};

    REQUIRE_STREQ(log_line_info.message_format(), "Test fmt {}");
    REQUIRE_EQ(log_line_info.level(), quill::LogLevel::Debug);
    REQUIRE_STREQ(log_line_info.lineno(), "15");
  }

  {
    constexpr LogRecordMetadata log_line_info{QUILL_STRINGIFY(__LINE__), __FILE__, __FUNCTION__,
                                              "Test another fmt {}", quill::LogLevel::Info};

    REQUIRE_STREQ(log_line_info.message_format(), "Test another fmt {}");
    REQUIRE_EQ(log_line_info.level(), quill::LogLevel::Info);
    REQUIRE_STREQ(log_line_info.lineno(), "24");
    REQUIRE_STREQ(log_line_info.filename(), "LogRecordMetadataTest.cpp");
    REQUIRE_STREQ(log_line_info.level_as_str(), "INFO     ");
  }
}

TEST_SUITE_END();