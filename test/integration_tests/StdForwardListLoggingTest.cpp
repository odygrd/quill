#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"
#include "quill/std/ForwardList.h"

#include <cstdio>
#include <forward_list>
#include <string>
#include <string_view>
#include <vector>

using namespace quill;

/***/
TEST_CASE("std_forward_list_logging")
{
  static constexpr char const* filename = "std_forward_list_logging.log";
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
    std::forward_list<char> c = {'a', 'c'};
    LOG_INFO(logger, "c {}", c);

    std::forward_list<short> si = {-12, 10};
    LOG_INFO(logger, "si {}", si);

    std::forward_list<int> i = {-123, 1};
    LOG_INFO(logger, "i {}", i);

    std::forward_list<long> li = {9876, 1232};
    LOG_INFO(logger, "li {}", li);

    std::forward_list<long long> lli = {321, 231};
    LOG_INFO(logger, "lli {}", lli);

    std::forward_list<unsigned short> usi = {15, 2};
    LOG_INFO(logger, "usi {}", usi);

    std::forward_list<unsigned int> ui = {123, 2};
    LOG_INFO(logger, "ui {}", ui);

    std::forward_list<unsigned long> uli = {3213, 2876};
    LOG_INFO(logger, "uli {}", uli);

    std::forward_list<unsigned long long> ulli = {321, 1321};
    LOG_INFO(logger, "ulli {}", ulli);

    std::forward_list<float> f = {111.1f, 323.31f};
    LOG_INFO(logger, "f {}", f);

    std::forward_list<double> d = {12.1, 3213213.123};
    LOG_INFO(logger, "d {}", d);

    std::forward_list<int> const& cri = i;
    LOG_INFO(logger, "cri {}", cri);

    std::forward_list<int>& ci = i;
    LOG_INFO(logger, "ci {}", ci);

    std::forward_list<std::string> sa = {"test", "string"};
    LOG_INFO(logger, "sa {}", sa);

    std::forward_list<std::string_view> sva = {"test", "string_view"};
    LOG_INFO(logger, "sva {}", sva);

    std::forward_list<char const*> scva = {"c style", "string test", "test", "log"};
    LOG_INFO(logger, "scva {}", scva);

    LOG_INFO(logger, "scva {} sa {} ulli {} scva {} sa {}", scva, sa, ulli, scva, sa);

    std::forward_list<int> empt;
    LOG_INFO(logger, "empt {}", empt);
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       c ['a', 'c']"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       si [-12, 10]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       i [-123, 1]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       li [9876, 1232]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       lli [321, 231]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       usi [15, 2]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       ui [123, 2]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       uli [3213, 2876]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       ulli [321, 1321]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       f [111.1, 323.31]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       d [12.1, 3213213.123]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       cri [-123, 1]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       ci [-123, 1]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       sa [\"test\", \"string\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       sva [\"test\", \"string_view\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       scva [\"c style\", \"string test\", \"test\", \"log\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       scva [\"c style\", \"string test\", \"test\", \"log\"] sa [\"test\", \"string\"] ulli [321, 1321] scva [\"c style\", \"string test\", \"test\", \"log\"] sa [\"test\", \"string\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       empt []"}));

  testing::remove_file(filename);
}