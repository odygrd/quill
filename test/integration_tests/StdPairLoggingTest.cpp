#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include "quill/std/Pair.h"

#include <array>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

using namespace quill;

/***/
TEST_CASE("std_pair_logging")
{
  static constexpr char const* filename = "std_pair_logging.log";
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
    std::pair<bool, int> b = {true, 312};
    LOG_INFO(logger, "v {}", b);

    std::pair<std::string, std::string> sa = {"test", "string"};
    LOG_INFO(logger, "sa {}", sa);

    std::pair<std::string_view, std::string> sva = {"test", "string_view"};
    LOG_INFO(logger, "sva {}", sva);

    std::pair<char const*, std::string> scva = {"c style", "string test"};
    LOG_INFO(logger, "scva {}", scva);

    std::pair<std::pair<std::string, std::string_view>, std::pair<const char*, uint32_t>> cp = {
      {"pair", "testing"}, {"first", 2}};
    LOG_INFO(logger, "cp {}", cp);

    std::pair<int, double> empt;
    LOG_INFO(logger, "empt {}", empt);
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       v (true, 312)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       sa (\"test\", \"string\")"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       sva (\"test\", \"string_view\")"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       scva (\"c style\", \"string test\")"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       cp ((\"pair\", \"testing\"), (\"first\", 2))"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       empt (0, 0)"}));

  testing::remove_file(filename);
}