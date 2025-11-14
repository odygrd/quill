#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include "quill/std/Tuple.h"

#include <array>
#include <cstdio>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

using namespace quill;

/***/
TEST_CASE("std_tuple_logging")
{
  static constexpr char const* filename = "std_tuple_logging.log";
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
    std::tuple<std::string> st{"123456789"};
    LOG_INFO(logger, "st {}", st);

    std::tuple<std::string, int> et;
    LOG_INFO(logger, "et {}", et);

    std::tuple<std::string, std::string_view, int, double, char const*, std::string, int> ct{
      "string", "string_view", 213, 33.12, "c_style", "another_string", 123};
    LOG_INFO(logger, "ct {}", ct);

    LOG_INFO(logger, "ct {} et {} st {}", ct, et, st);

    // Test rvalue references with tuples
    std::tuple<int, std::string, double> rvalue_tuple = {100, "hundred", 1.5};
    LOG_INFO(logger, "rvalue_tuple {}", std::move(rvalue_tuple));

    // Test with temporary tuple
    LOG_INFO(logger, "temp_tuple {}", std::tuple<int, std::string>{200, "temp"});
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       st (\"123456789\")"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       et (\"\", 0)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       ct (\"string\", \"string_view\", 213, 33.12, \"c_style\", \"another_string\", 123)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       ct (\"string\", \"string_view\", 213, 33.12, \"c_style\", \"another_string\", 123) et (\"\", 0) st (\"123456789\")"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       rvalue_tuple (100, \"hundred\", 1.5)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       temp_tuple (200, \"temp\")"}));

  testing::remove_file(filename);
}