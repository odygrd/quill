#include "quill/detail/record/LogRecordMetadata.h"
#include "quill/detail/misc/Macros.h"
#include <gtest/gtest.h>

using namespace quill::detail;
using namespace quill;

TEST(LogRecordMetadata, construct)
{
  {
    constexpr LogRecordMetadata log_line_info{QUILL_STRINGIFY(__LINE__), __FILE__, __FUNCTION__,
                                              "Test fmt {}", quill::LogLevel::Debug};

    EXPECT_STREQ(log_line_info.message_format(), "Test fmt {}");
    EXPECT_EQ(log_line_info.level(), quill::LogLevel::Debug);
    EXPECT_STREQ(log_line_info.lineno(), "11");
  }

  {
    constexpr LogRecordMetadata log_line_info{QUILL_STRINGIFY(__LINE__), __FILE__, __FUNCTION__,
                                              "Test another fmt {}", quill::LogLevel::Info};

    EXPECT_STREQ(log_line_info.message_format(), "Test another fmt {}");
    EXPECT_EQ(log_line_info.level(), quill::LogLevel::Info);
    EXPECT_STREQ(log_line_info.lineno(), "20");
    EXPECT_STREQ(log_line_info.filename(), "LogRecordMetadataTest.cpp");
    EXPECT_STREQ(log_line_info.level_as_str(), "INFO    ");
  }
}