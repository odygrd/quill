#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/bundled/fmt/ostream.h"
#include "quill/sinks/FileSink.h"
#include "quill/std/Array.h"

#include <cstdio>
#include <string>
#include <string_view>
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

std::ostream& operator<<(std::ostream& os, TestEnumClass const& test_enum_class)
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
TEST_CASE("c_array_types_logging")
{
  static constexpr char const* filename = "c_array_types_logging.log";
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

  {
    std::string sra[2] = {"test_1", "test_2"};
    LOG_INFO(logger, "sra [{}]", sra);

    std::string_view svra[2] = {"test_21", "test_22"};
    LOG_INFO(logger, "svra [{}]", svra);

    bool a[2] = {true, false};
    LOG_INFO(logger, "a {}", a);

    int b[6] = {123, 456, 789, 321, 654, 987};
    LOG_INFO(logger, "b {}", b);

    double const c[2] = {123.321, 0};
    LOG_INFO(logger, "c {}", c);

    auto& d = b;
    LOG_INFO(logger, "d {}", d);

    auto& e = c;
    LOG_INFO(logger, "e {}", e);

    TestEnum t{TestEnum::Test2};
    LOG_INFO(logger, "t {}", t);

    TestEnum ta[2]{TestEnum::Test2, TestEnum::Test1};
    LOG_INFO(logger, "ta {}", ta);

    TestEnumClass tc{TestEnumClass::Test5};
    LOG_INFO(logger, "tc {}", tc);

    TestEnumClass tca[2]{TestEnumClass::Test5, TestEnumClass::Test4};
    LOG_INFO(logger, "tca {}", tca);

    std::string y{"test"};
    int x = 111;
    LOG_INFO(logger, "a b c y d e x t ta tc tca {} {} {} {} {} {} {} {} {} {} {}", a, b, c, y, d, e,
             x, t, ta, tc, tca);
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       sra [[\"test_1\", \"test_2\"]]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       svra [[\"test_21\", \"test_22\"]]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       a [true, false]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       b [123, 456, 789, 321, 654, 987]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       c [123.321, 0]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       d [123, 456, 789, 321, 654, 987]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       e [123.321, 0]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       t Test2"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       ta [Test2, Test1]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       tc Test5"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       tca [Test5, Test4]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       a b c y d e x t ta tc tca [true, false] [123, 456, 789, 321, 654, 987] [123.321, 0] test [123, 456, 789, 321, 654, 987] [123.321, 0] 111 Test2 [Test2, Test1] Test5 [Test5, Test4]"}));

  testing::remove_file(filename);
}