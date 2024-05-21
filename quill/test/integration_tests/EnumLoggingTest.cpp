#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/bundled/fmt/format.h"
#include "quill/bundled/fmt/ostream.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace quill;

enum TestEnum : int
{
  Test1 = 1,
  Test2 = 2,
  Test3 = 3
};

std::ostream& operator<<(std::ostream& os, TestEnum const& test_enum)
{
  switch (test_enum)
  {
  case TestEnum::Test1:
    os << "Test1";
    break;
  case TestEnum::Test2:
    os << "Test2";
    break;
  case TestEnum::Test3:
    os << "Test3";
    break;
  default:
    os << "Unknown";
    break;
  }
  return os;
}

template <>
struct fmtquill::formatter<TestEnum> : fmtquill::ostream_formatter
{
};

enum class TestEnumClass : uint64_t
{
  Test4 = 4,
  Test5 = 5,
  Test6 = 6
};

std::ostream& operator<<(std::ostream& os, const TestEnumClass& test_enum_class)
{
  switch (test_enum_class)
  {
  case TestEnumClass::Test4:
    os << "Test4";
    break;
  case TestEnumClass::Test5:
    os << "Test5";
    break;
  case TestEnumClass::Test6:
    os << "Test6";
    break;
  default:
    os << "Unknown";
    break;
  }
  return os;
}

template <>
struct fmtquill::formatter<TestEnumClass> : fmtquill::ostream_formatter
{
};

/***/
TEST_CASE("enum_logging")
{
  static constexpr char const* filename = "enum_logging.log";
  static std::string const logger_name = "logger";

  // Start the logging backend thread
  Backend::start();

  Frontend::preallocate();

  // Set writing logging to a file
  auto file_sink = Frontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));

  LOG_INFO(logger, "{} -> {}, {} -> {}", TestEnum::Test1, static_cast<int>(TestEnum::Test1),
           TestEnumClass::Test4, static_cast<uint64_t>(TestEnumClass::Test4));

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  std::string expected_string = "LOG_INFO      " + logger_name + "       Test1 -> 1, Test4 -> 4";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string));

  testing::remove_file(filename);
}