#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"
#include "quill/std/UnorderedMap.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using namespace quill;

/***/
TEST_CASE("std_unordered_map_logging")
{
  static constexpr char const* filename = "std_unordered_map_logging.log";
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
    std::unordered_map<int, double> idm = {{111, 3213.21}};
    LOG_INFO(logger, "idm {}", idm);

    std::unordered_map<int, std::string> loopv;
    for (int iter = 0; iter < 25; ++iter)
    {
      loopv.emplace(iter, std::to_string(iter));
    }
    LOG_INFO(logger, "loopv {}", loopv);

    std::unordered_map<std::string, std::string> loopsv;
    for (int iter = 0; iter < 25; ++iter)
    {
      loopsv.emplace(std::to_string(iter), std::to_string(iter * 2));
    }
    LOG_INFO(logger, "loopsv {}", loopv);

    std::unordered_map<char const*, char const*> ccm = {{"4", "400"}};
    LOG_INFO(logger, "ccm {}", ccm);

    std::unordered_map<char const*, int> cim = {{"4", 4}};
    LOG_INFO(logger, "cim {}", cim);
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       idm {111: 3213.21}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       loopv {"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       loopsv {"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       ccm {\"4\": \"400\"}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       cim {\"4\": 4}"}));

  testing::remove_file(filename);
}