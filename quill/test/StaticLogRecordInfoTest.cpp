#include "quill/detail/record/StaticLogRecordInfo.h"

#include <gtest/gtest.h>

using namespace quill::detail;
using namespace quill;

TEST(StaticLogRecordInfo, construct)
{
  {
    constexpr StaticLogRecordInfo log_line_info{__LINE__, __FILE__, __FUNCTION__, "Test fmt {}",
                                                quill::LogLevel::Debug};

    EXPECT_STREQ(log_line_info.message_format(), "Test fmt {}");
    EXPECT_EQ(log_line_info.level(), quill::LogLevel::Debug);
    EXPECT_EQ(log_line_info.lineno(), 11u);
  }

  {
    constexpr StaticLogRecordInfo log_line_info{__LINE__, __FILE__, __FUNCTION__,
                                                "Test another fmt {}", quill::LogLevel::Info};

    EXPECT_STREQ(log_line_info.message_format(), "Test another fmt {}");
    EXPECT_EQ(log_line_info.level(), quill::LogLevel::Info);
    EXPECT_EQ(log_line_info.lineno(), 20u);
    EXPECT_STREQ(log_line_info.filename(), "StaticLogRecordInfoTest.cpp");
    EXPECT_STREQ(log_line_info.level_as_str(), "LOG_INFO    ");
  }
}