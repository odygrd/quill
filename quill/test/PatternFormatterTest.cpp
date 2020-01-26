#include <gtest/gtest.h>

#include "quill/detail/PatternFormatter.h"
#include <chrono>
using namespace quill::detail;
using namespace quill;

TEST(PatternFormatter, default_pattern_formatter)
{
  PatternFormatter default_pattern_formatter;

  uint64_t const ts = 1579815761000023000;
  uint32_t const thread_id = 31341;
  std::string const logger_name = "test_logger";
  StaticLogRecordInfo log_line_info{__LINE__, __FILE__, __func__, "This the {} formatter {}", LogLevel::Info};

  // Format to a buffer
  fmt::memory_buffer formatted_buffer =
    default_pattern_formatter.format(ts, thread_id, logger_name, log_line_info, "pattern", 1234);

  // Convert the buffer to a string
  std::string const formatted_string = fmt::to_string(formatted_buffer);

  std::string const expected_string =
    "1579815761000023000 [31341] PatternFormatterTest.cpp:15 LOG_INFO     test_logger - This the "
    "pattern formatter 1234\n";

  EXPECT_EQ(formatted_buffer.size(), expected_string.length());
  EXPECT_EQ(formatted_string, expected_string);
}

TEST(PatternFormatter, custom_pattern)
{
  // Custom pattern with part 1 and part 3
  {
    PatternFormatter custom_pattern_formatter{
      QUILL_STRING("%(ascii_time) [%(thread)] %(filename):%(lineno) %(level_name) %(logger_name) - "
                   "%(message) [%(function_name)]")};

    uint64_t const ts = 1579815761000023000;
    uint32_t const thread_id = 31341;
    std::string const logger_name = "test_logger";
    StaticLogRecordInfo log_line_info{__LINE__, __FILE__, __func__, "This the {1} formatter {0}", LogLevel::Debug};

    // Format to a buffer
    fmt::memory_buffer formatted_buffer =
      custom_pattern_formatter.format(ts, thread_id, logger_name, log_line_info, "pattern", 1234);

    // Convert the buffer to a string
    std::string const formatted_string = fmt::to_string(formatted_buffer);

    std::string const expected_string =
      "1579815761000023000 [31341] PatternFormatterTest.cpp:43 LOG_DEBUG    test_logger - This the "
      "1234 formatter pattern [TestBody]\n";

    EXPECT_EQ(formatted_buffer.size(), expected_string.length());
    EXPECT_EQ(formatted_string, expected_string);
  }

  // Message only
  {
    PatternFormatter custom_pattern_formatter{QUILL_STRING("%(message)")};

    uint64_t const ts = 1579815761000023000;
    uint32_t const thread_id = 31341;
    std::string const logger_name = "test_logger";
    StaticLogRecordInfo log_line_info{__LINE__, __FILE__, __func__, "This the {1} formatter {0}", LogLevel::Debug};

    // Format to a buffer
    fmt::memory_buffer formatted_buffer =
      custom_pattern_formatter.format(ts, thread_id, logger_name, log_line_info, "pattern", 12.34);

    // Convert the buffer to a string
    std::string const formatted_string = fmt::to_string(formatted_buffer);

    std::string const expected_string = "This the 12.34 formatter pattern\n";

    EXPECT_EQ(formatted_buffer.size(), expected_string.length());
    EXPECT_EQ(formatted_string, expected_string);
  }
}

TEST(PatternFormatter, invalid_pattern)
{
  EXPECT_THROW(PatternFormatter{QUILL_STRING(
                 "%(ascii_time) [%(thread)] %(filename):%(lineno) %(level_name) %(logger_name) - "
                 "[%(function_name)]")},
               std::runtime_error);
}

TEST(PatternFormatter, copy_constructor)
{
  PatternFormatter custom_pattern_formatter{
    QUILL_STRING("%(ascii_time) [%(thread)] %(filename):%(lineno) %(level_name) %(logger_name) - "
                 "%(message) [%(function_name)]")};

  {
    PatternFormatter custom_pattern_formatter_copy{custom_pattern_formatter};

    uint64_t const ts = 1579815761000023000;
    uint32_t const thread_id = 31341;
    std::string const logger_name = "test_logger";
    StaticLogRecordInfo log_line_info{__LINE__, __FILE__, __func__, "This the {1} formatter {0}", LogLevel::Debug};

    // Format to a buffer
    fmt::memory_buffer formatted_buffer =
      custom_pattern_formatter_copy.format(ts, thread_id, logger_name, log_line_info, "pattern", 1234);

    // Convert the buffer to a string
    std::string const formatted_string = fmt::to_string(formatted_buffer);

    std::string const expected_string =
      "1579815761000023000 [31341] PatternFormatterTest.cpp:103 LOG_DEBUG    test_logger - This "
      "the "
      "1234 formatter pattern [TestBody]\n";

    EXPECT_EQ(formatted_buffer.size(), expected_string.length());
    EXPECT_EQ(formatted_string, expected_string);
  }

  {
    // without part 1 and part 3
    PatternFormatter custom_pattern_formatter_2{QUILL_STRING("%(message)")};

    PatternFormatter custom_pattern_formatter_copy_2{custom_pattern_formatter_2};
  }
}

TEST(PatternFormatter, move_constructor)
{
  PatternFormatter custom_pattern_formatter{
    QUILL_STRING("%(ascii_time) [%(thread)] %(filename):%(lineno) %(level_name) %(logger_name) - "
                 "%(message) [%(function_name)]")};

  {
    PatternFormatter custom_pattern_formatter_copy{std::move(custom_pattern_formatter)};

    uint64_t const ts = 1579815761000023000;
    uint32_t const thread_id = 31341;
    std::string const logger_name = "test_logger";
    StaticLogRecordInfo log_line_info{__LINE__, __FILE__, __func__, "This the {1} formatter {0}", LogLevel::Debug};

    // Format to a buffer
    fmt::memory_buffer formatted_buffer =
      custom_pattern_formatter_copy.format(ts, thread_id, logger_name, log_line_info, "pattern", 1234);

    // Convert the buffer to a string
    std::string const formatted_string = fmt::to_string(formatted_buffer);

    std::string const expected_string =
      "1579815761000023000 [31341] PatternFormatterTest.cpp:141 LOG_DEBUG    test_logger - This "
      "the "
      "1234 formatter pattern [TestBody]\n";

    EXPECT_EQ(formatted_buffer.size(), expected_string.length());
    EXPECT_EQ(formatted_string, expected_string);
  }
}

TEST(PatternFormatter, copy_assignment_operator)
{
  PatternFormatter custom_pattern_formatter{
    QUILL_STRING("%(ascii_time) [%(thread)] %(filename):%(lineno) %(level_name) %(logger_name) - "
                 "%(message) [%(function_name)]")};

  {
    PatternFormatter custom_pattern_formatter_copy;
    custom_pattern_formatter_copy = custom_pattern_formatter;

    uint64_t const ts = 1579815761000023000;
    uint32_t const thread_id = 31341;
    std::string const logger_name = "test_logger";
    StaticLogRecordInfo log_line_info{__LINE__, __FILE__, __func__, "This the {1} formatter {0}", LogLevel::Debug};

    // Format to a buffer
    fmt::memory_buffer formatted_buffer =
      custom_pattern_formatter_copy.format(ts, thread_id, logger_name, log_line_info, "pattern", 1234);

    // Convert the buffer to a string
    std::string const formatted_string = fmt::to_string(formatted_buffer);

    std::string const expected_string =
      "1579815761000023000 [31341] PatternFormatterTest.cpp:173 LOG_DEBUG    test_logger - This "
      "the "
      "1234 formatter pattern [TestBody]\n";

    EXPECT_EQ(formatted_buffer.size(), expected_string.length());
    EXPECT_EQ(formatted_string, expected_string);
  }

  {
    // without part 1 and part 3
    PatternFormatter custom_pattern_formatter_2{QUILL_STRING("%(message)")};

    PatternFormatter custom_pattern_formatter_copy_2;
    custom_pattern_formatter_copy_2 = custom_pattern_formatter_2;
  }
}

TEST(PatternFormatter, move_assignment_operator)
{
  PatternFormatter custom_pattern_formatter{
    QUILL_STRING("%(ascii_time) [%(thread)] %(filename):%(lineno) %(level_name) %(logger_name) - "
                 "%(message) [%(function_name)]")};

  {
    PatternFormatter custom_pattern_formatter_copy;
    custom_pattern_formatter_copy = std::move(custom_pattern_formatter);

    uint64_t const ts = 1579815761000023000;
    uint32_t const thread_id = 31341;
    std::string const logger_name = "test_logger";
    StaticLogRecordInfo log_line_info{__LINE__, __FILE__, __func__, "This the {1} formatter {0}", LogLevel::Debug};

    // Format to a buffer
    fmt::memory_buffer formatted_buffer =
      custom_pattern_formatter_copy.format(ts, thread_id, logger_name, log_line_info, "pattern", 1234);

    // Convert the buffer to a string
    std::string const formatted_string = fmt::to_string(formatted_buffer);

    std::string const expected_string =
      "1579815761000023000 [31341] PatternFormatterTest.cpp:213 LOG_DEBUG    test_logger - This "
      "the 1234 formatter pattern [TestBody]\n";

    EXPECT_EQ(formatted_buffer.size(), expected_string.length());
    EXPECT_EQ(formatted_string, expected_string);
  }
}