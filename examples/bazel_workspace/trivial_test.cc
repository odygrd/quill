#include "gtest/gtest.h"

#include "quill/LogLevel.h"
#include "quill/QuillError.h"

using namespace quill;

TEST(TrivialTest, ToStringMapsLevelToStringName)
{
  {
    LogLevel log_level{LogLevel::None};
    EXPECT_STREQ(to_string(log_level), "None");
  }

  {
    LogLevel log_level{LogLevel::Backtrace};
    EXPECT_STREQ(to_string(log_level), "Backtrace");
  }

  {
    LogLevel log_level{LogLevel::Critical};
    EXPECT_STREQ(to_string(log_level), "Critical");
  }

  {
    LogLevel log_level{LogLevel::Error};
    EXPECT_STREQ(to_string(log_level), "Error");
  }

  {
    LogLevel log_level{LogLevel::Warning};
    EXPECT_STREQ(to_string(log_level), "Warning");
  }

  {
    LogLevel log_level{LogLevel::Info};
    EXPECT_STREQ(to_string(log_level), "Info");
  }

  {
    LogLevel log_level{LogLevel::Debug};
    EXPECT_STREQ(to_string(log_level), "Debug");
  }

  {
    LogLevel log_level{LogLevel::TraceL1};
    EXPECT_STREQ(to_string(log_level), "TraceL1");
  }

  {
    LogLevel log_level{LogLevel::TraceL2};
    EXPECT_STREQ(to_string(log_level), "TraceL2");
  }

  {
    LogLevel log_level{LogLevel::TraceL3};
    EXPECT_STREQ(to_string(log_level), "TraceL3");
  }

  {
#ifndef QUILL_NO_EXCEPTIONS
    LogLevel log_level;
    log_level = static_cast<LogLevel>(-1);
    EXPECT_THROW(QUILL_MAYBE_UNUSED auto s = to_string(log_level), quill::QuillError);
#endif
  }
}

TEST(TrivialTest, FromStringMapsStringToLevel)
{
  {
    std::string log_level{"None"};
    EXPECT_EQ(from_string(log_level), LogLevel::None);
  }

  {
    std::string log_level{"Backtrace"};
    EXPECT_EQ(from_string(log_level), LogLevel::Backtrace);
  }

  {
    std::string log_level{"Critical"};
    EXPECT_EQ(from_string(log_level), LogLevel::Critical);
  }

  {
    std::string log_level{"Error"};
    EXPECT_EQ(from_string(log_level), LogLevel::Error);
  }

  {
    std::string log_level{"Warning"};
    EXPECT_EQ(from_string(log_level), LogLevel::Warning);
  }

  {
    std::string log_level{"Info"};
    EXPECT_EQ(from_string(log_level), LogLevel::Info);
  }

  {
    std::string log_level{"Debug"};
    EXPECT_EQ(from_string(log_level), LogLevel::Debug);
  }

  {
    std::string log_level{"TraceL1"};
    EXPECT_EQ(from_string(log_level), LogLevel::TraceL1);
  }

  {
    std::string log_level{"TraceL2"};
    EXPECT_EQ(from_string(log_level), LogLevel::TraceL2);
  }

  {
    std::string log_level{"TraceL3"};
    EXPECT_EQ(from_string(log_level), LogLevel::TraceL3);
  }

  {
#ifndef QUILL_NO_EXCEPTIONS
    std::string log_level{"dummy"};
    EXPECT_THROW(QUILL_MAYBE_UNUSED auto res = from_string(log_level), quill::QuillError);
#endif
  }
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
