#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"
#include "quill/std/UnorderedSet.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

using namespace quill;

/***/
TEST_CASE("std_unordered_multi_set_logging")
{
  static constexpr char const* filename = "std_unordered_multi_set_logging.log";
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
    std::unordered_multiset<char> c = {'a'};
    LOG_INFO(logger, "c {}", c);

    std::unordered_multiset<short> si = {-12};
    LOG_INFO(logger, "si {}", si);

    std::unordered_multiset<int> i = {-123};
    LOG_INFO(logger, "i {}", i);

    std::unordered_multiset<long> li = {9876};
    LOG_INFO(logger, "li {}", li);

    std::unordered_multiset<long long> lli = {321};
    LOG_INFO(logger, "lli {}", lli);

    std::unordered_multiset<unsigned short> usi = {15};
    LOG_INFO(logger, "usi {}", usi);

    std::unordered_multiset<unsigned int> ui = {123};
    LOG_INFO(logger, "ui {}", ui);

    std::unordered_multiset<unsigned long> uli = {3213};
    LOG_INFO(logger, "uli {}", uli);

    std::unordered_multiset<unsigned long long> ulli = {321};
    LOG_INFO(logger, "ulli {}", ulli);

    std::unordered_multiset<float> f = {111.1f};
    LOG_INFO(logger, "f {}", f);

    std::unordered_multiset<double> d = {12.1};
    LOG_INFO(logger, "d {}", d);

    std::unordered_multiset<int> const& cri = i;
    LOG_INFO(logger, "cri {}", cri);

    std::unordered_multiset<int>& ci = i;
    LOG_INFO(logger, "ci {}", ci);

    std::unordered_multiset<std::string> sa = {"test"};
    LOG_INFO(logger, "sa {}", sa);

    std::unordered_multiset<std::string_view> sva = {"string_view"};
    LOG_INFO(logger, "sva {}", sva);

    std::unordered_multiset<char const*> scva = {"c_style"};
    LOG_INFO(logger, "scva {}", scva);

    LOG_INFO(logger, "scva {} sa {} ulli {} scva {} sa {}", scva, sa, ulli, scva, sa);

    std::unordered_multiset<int> loopv;
    for (int iter = 0; iter < 100; ++iter)
    {
      loopv.insert(iter);
    }

    LOG_INFO(logger, "loopv {}", loopv);

    std::unordered_multiset<std::string> loopsv;
    for (int iter = 0; iter < 100; ++iter)
    {
      loopsv.insert(std::to_string(iter));
    }

    LOG_INFO(logger, "loopsv {}", loopsv);

    std::unordered_multiset<int> empt;
    LOG_INFO(logger, "empt {}", empt);
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       c {'a'}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       si {-12}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       i {-123}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       li {9876}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       lli {321}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       usi {15}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       ui {123}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       uli {3213}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       ulli {321}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       f {111.1}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       d {12.1}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       cri {-123}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       ci {-123}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       sa {\"test\"}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       sva {\"string_view\"}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       scva {\"c_style\"}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       scva {\"c_style\"} sa {\"test\"} ulli {321} scva {\"c_style\"} sa {\"test\"}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       loopv {"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       loopsv {"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       empt {}"}));

  testing::remove_file(filename);
}