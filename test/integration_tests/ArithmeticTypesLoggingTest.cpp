#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

using namespace quill;

/***/
TEST_CASE("arithmetic_types_logging")
{
  static constexpr char const* filename = "arithmetic_types_logging.log";
  static std::string const logger_name = "logger";

  // Start the logging backend thread
  BackendOptions bo;
  bo.error_notifier = [](std::string const&) {};
  Backend::start(bo);

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
    bool b = true;
    LOG_INFO(logger, "v [{}]", b);

    char c = 'a';
    LOG_INFO(logger, "c [{}]", c);

    short int si = -12;
    LOG_INFO(logger, "si [{}]", si);

    int i = -123;
    LOG_INFO(logger, "i [{}]", i);

    long int li = 9876;
    LOG_INFO(logger, "li [{}]", li);

    long long int lli = 321;
    LOG_INFO(logger, "lli [{}]", lli);

    unsigned short int usi = 15;
    LOG_INFO(logger, "usi [{}]", usi);

    unsigned int ui = 123;
    LOG_INFO(logger, "ui [{}]", ui);

    unsigned long int uli = 2876;
    LOG_INFO(logger, "uli [{}]", uli);

    unsigned long long int ulli = 1321;
    LOG_INFO(logger, "ulli [{}]", ulli);

    float f = 323.31f;
    LOG_INFO(logger, "f [{}]", f);

    double d = 3213213.123;
    LOG_INFO(logger, "d [{}]", d);

    int const& cri = i;
    LOG_INFO(logger, "cri [{}]", cri);

    int& ci = i;
    LOG_INFO(logger, "ci [{}]", ci);

    LOG_INFO(logger, "invalid format [{%f}]", 321.1);
  }

  int v = 111;
  int* ptr_test = &v;
  LOG_INFO(logger, "pointer [{}]", static_cast<void const*>(ptr_test));

  void* void_ptr{nullptr};
  void const* void_const_ptr{nullptr};

  LOG_INFO(logger, "void pointer [{}]", void_ptr);
  LOG_INFO(logger, "void const pointer [{}]", void_const_ptr);

  // Test rvalue references to ensure forwarding works correctly
  int rvalue_int = 999;
  LOG_INFO(logger, "rvalue int [{}]", std::move(rvalue_int));

  LOG_INFO(logger, "rvalue int [{}]", 1234);

  std::string rvalue_str = "rvalue_test";
  LOG_INFO(logger, "rvalue string [{}]", std::move(rvalue_str));

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();
  REQUIRE_FALSE(Backend::is_running());

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       v [true]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       c [a]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       si [-12]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       i [-123]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       li [9876]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       lli [321]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       usi [15]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       ui [123]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       uli [2876]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       ulli [1321]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       f [323.31]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       d [3213213.123]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       cri [-123]"}));

  {
    std::string expected_ptr_value_str{"LOG_INFO      " + logger_name + "       pointer ["};
    expected_ptr_value_str += fmtquill::format("{}", fmtquill::ptr(ptr_test));
    expected_ptr_value_str += "]";
    REQUIRE(quill::testing::file_contains(file_contents, expected_ptr_value_str));
  }

  {
    std::string expected_ptr_value_str{"LOG_INFO      " + logger_name + "       void pointer ["};
    expected_ptr_value_str += fmtquill::format("{}", fmtquill::ptr(void_ptr));
    expected_ptr_value_str += "]";
    REQUIRE(quill::testing::file_contains(file_contents, expected_ptr_value_str));
  }

  {
    std::string expected_ptr_value_str{"LOG_INFO      " + logger_name +
                                       "       void const pointer ["};
    expected_ptr_value_str += fmtquill::format("{}", fmtquill::ptr(void_const_ptr));
    expected_ptr_value_str += "]";
    REQUIRE(quill::testing::file_contains(file_contents, expected_ptr_value_str));
  }

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       rvalue int [999]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       rvalue int [1234]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       rvalue string [rvalue_test]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents,
    std::string{"LOG_INFO      " + logger_name +
                "       [Could not format log statement. message: \"invalid format [{%f}]\""}));

  testing::remove_file(filename);
}