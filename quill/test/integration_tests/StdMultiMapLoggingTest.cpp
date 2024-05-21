#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"
#include "quill/std/Map.h"

#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <string_view>
#include <vector>

using namespace quill;

struct CStringComparator
{
  bool operator()(char const* a, char const* b) const { return std::strcmp(a, b) < 0; }
};

/***/
TEST_CASE("std_multimap_logging")
{
  static constexpr char const* filename = "std_multimap_logging.log";
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
    std::multimap<int, double> idm = {{111, 3213.21}, {222, 321.19}, {333, 5555.99}};
    LOG_INFO(logger, "idm {}", idm);

    std::multimap<int, std::string> loopv;
    for (int iter = 0; iter < 25; ++iter)
    {
      loopv.emplace(iter, std::to_string(iter));
    }
    LOG_INFO(logger, "loopv {}", loopv);

    std::multimap<std::string, std::string> loopsv;
    for (int iter = 0; iter < 25; ++iter)
    {
      loopsv.emplace(std::to_string(iter), std::to_string(iter * 2));
    }
    LOG_INFO(logger, "loopsv {}", loopv);

    std::multimap<char const*, char const*, CStringComparator> ccm = {
      {"4", "400"}, {"3", "300"}, {"1", "100"}, {"2", "200"}};
    LOG_INFO(logger, "ccm {}", ccm);

    std::multimap<char const*, int, CStringComparator> cim = {{"4", 4}, {"3", 3}, {"1", 1}, {"2", 2}};
    LOG_INFO(logger, "cim {}", cim);
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       idm {111: 3213.21, 222: 321.19, 333: 5555.99}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       loopv {0: \"0\", 1: \"1\", 2: \"2\", 3: \"3\", 4: \"4\", 5: \"5\", 6: \"6\", 7: \"7\", 8: \"8\", 9: \"9\", 10: \"10\", 11: \"11\", 12: \"12\", 13: \"13\", 14: \"14\", 15: \"15\", 16: \"16\", 17: \"17\", 18: \"18\", 19: \"19\", 20: \"20\", 21: \"21\", 22: \"22\", 23: \"23\", 24: \"24\"}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       loopsv {0: \"0\", 1: \"1\", 2: \"2\", 3: \"3\", 4: \"4\", 5: \"5\", 6: \"6\", 7: \"7\", 8: \"8\", 9: \"9\", 10: \"10\", 11: \"11\", 12: \"12\", 13: \"13\", 14: \"14\", 15: \"15\", 16: \"16\", 17: \"17\", 18: \"18\", 19: \"19\", 20: \"20\", 21: \"21\", 22: \"22\", 23: \"23\", 24: \"24\""}));

  REQUIRE(quill::testing::file_contains(
    file_contents,
    std::string{"LOG_INFO      " + logger_name +
                "       ccm {\"1\": \"100\", \"2\": \"200\", \"3\": \"300\", \"4\": \"400\"}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       cim {\"1\": 1, \"2\": 2, \"3\": 3, \"4\": 4}"}));

  testing::remove_file(filename);
}