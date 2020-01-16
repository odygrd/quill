#include "quill/detail/LogLineInfo.h"

#include <gtest/gtest.h>

using namespace quill::detail;
using namespace quill;

TEST(LogLineInfo, construct)
{
  {
    constexpr LogLineInfo log_line_info{__LINE__, __FILE__, __FUNCTION__, "Test fmt {}", quill::LogLevel::Debug};

    EXPECT_STREQ(log_line_info.format(), "Test fmt {}");
    EXPECT_EQ(log_line_info.log_level(), quill::LogLevel::Debug);
    EXPECT_EQ(log_line_info.line(), 11u);
  }

  {
    constexpr LogLineInfo log_line_info{__LINE__, __FILE__, __FUNCTION__, "Test another fmt {}",
                                        quill::LogLevel::Info};

    EXPECT_STREQ(log_line_info.format(), "Test another fmt {}");
    EXPECT_EQ(log_line_info.log_level(), quill::LogLevel::Info);
    EXPECT_EQ(log_line_info.line(), 19u);
    EXPECT_STREQ(log_line_info.file_name(), "LogLineInfoTest.cpp");
    EXPECT_STREQ(log_line_info.log_level_str(), "LOG_INFO");
  }
}