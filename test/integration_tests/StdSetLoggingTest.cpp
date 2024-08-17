#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"
#include "quill/std/Set.h"

#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <string_view>
#include <vector>

using namespace quill;

struct CStringComparator
{
  bool operator()(char const* a, char const* b) const { return std::strcmp(a, b) < 0; }
};

/***/
TEST_CASE("std_set_logging")
{
  static constexpr char const* filename = "std_set_logging.log";
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
    std::set<char> c = {'a', 'c'};
    LOG_INFO(logger, "c {}", c);

    std::set<short> si = {-12, 10};
    LOG_INFO(logger, "si {}", si);

    std::set<int> i = {-123, 1};
    LOG_INFO(logger, "i {}", i);

    std::set<long> li = {9876, 1232};
    LOG_INFO(logger, "li {}", li);

    std::set<long long> lli = {321, 231};
    LOG_INFO(logger, "lli {}", lli);

    std::set<unsigned short> usi = {15, 2};
    LOG_INFO(logger, "usi {}", usi);

    std::set<unsigned int> ui = {123, 2};
    LOG_INFO(logger, "ui {}", ui);

    std::set<unsigned long> uli = {3213, 2876};
    LOG_INFO(logger, "uli {}", uli);

    std::set<unsigned long long> ulli = {321, 1321};
    LOG_INFO(logger, "ulli {}", ulli);

    std::set<float> f = {111.1f, 323.31f};
    LOG_INFO(logger, "f {}", f);

    std::set<double> d = {12.1, 3213213.123};
    LOG_INFO(logger, "d {}", d);

    std::set<int> const& cri = i;
    LOG_INFO(logger, "cri {}", cri);

    std::set<int>& ci = i;
    LOG_INFO(logger, "ci {}", ci);

    std::set<std::string> sa = {"test", "string"};
    LOG_INFO(logger, "sa {}", sa);

    std::set<std::string_view> sva = {"test", "string_view"};
    LOG_INFO(logger, "sva {}", sva);

    std::set<char const*, CStringComparator> scva = {"c_style", "aa", "string_test", "test", "log"};
    LOG_INFO(logger, "scva {}", scva);

    std::set<std::set<int>> aai = {{{321, 123}, {444, 333}, {111, 222}}};
    LOG_INFO(logger, "aai {}", aai);

    {
      std::set<std::set<char const*, CStringComparator>> aacs = {
        {{"one", "two"}, {"three", "four"}, {"five", "six"}}};
      LOG_INFO(logger, "aacs {}", aacs);
    }

    {
      std::set<std::set<std::set<char const*, CStringComparator>>> aaacs = {
        {{{{"one", "two"}, {"three", "four"}}},
         {{{"five", "six"}, {"seven", "eight"}}},
         {{{"nine", "ten"}, {"eleven", "twelve"}}}}};
      LOG_INFO(logger, "aaacs {}", aaacs);
    }

    LOG_INFO(logger, "scva {} sa {} ulli {} scva {} sa {} aai {}", scva, sa, ulli, scva, sa, aai);

    std::set<int> loopv;
    for (int iter = 0; iter < 100; ++iter)
    {
      loopv.insert(iter);
    }

    LOG_INFO(logger, "loopv {}", loopv);

    std::set<std::string> loopsv;
    for (int iter = 0; iter < 100; ++iter)
    {
      loopsv.insert(std::to_string(iter));
    }

    LOG_INFO(logger, "loopsv {}", loopsv);

    std::set<int> empt;
    LOG_INFO(logger, "empt {}", empt);
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       c {'a', 'c'}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       si {-12, 10}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       i {-123, 1}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       li {1232, 9876}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       lli {231, 321}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       usi {2, 15}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       ui {2, 123}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       uli {2876, 3213}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       ulli {321, 1321}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       f {111.1, 323.31}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       d {12.1, 3213213.123}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       cri {-123, 1}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       ci {-123, 1}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       sa {\"string\", \"test\"}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       sva {\"string_view\", \"test\"}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents,
    std::string{"LOG_INFO      " + logger_name +
                "       scva {\"aa\", \"c_style\", \"log\", \"string_test\", \"test\"}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       aai {{111, 222}, {123, 321}, {333, 444}}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       aacs {{"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       aaacs {{{"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       scva {\"aa\", \"c_style\", \"log\", \"string_test\", \"test\"} sa {\"string\", \"test\"} ulli {321, 1321} scva {\"aa\", \"c_style\", \"log\", \"string_test\", \"test\"} sa {\"string\", \"test\"} aai {{111, 222}, {123, 321}, {333, 444}}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       loopv {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       loopsv {\"0\", \"1\", \"10\", \"11\", \"12\", \"13\", \"14\", \"15\", \"16\", \"17\", \"18\", \"19\", \"2\", \"20\", \"21\", \"22\", \"23\", \"24\", \"25\", \"26\", \"27\", \"28\", \"29\", \"3\", \"30\", \"31\", \"32\", \"33\", \"34\", \"35\", \"36\", \"37\", \"38\", \"39\", \"4\", \"40\", \"41\", \"42\", \"43\", \"44\", \"45\", \"46\", \"47\", \"48\", \"49\", \"5\", \"50\", \"51\", \"52\", \"53\", \"54\", \"55\", \"56\", \"57\", \"58\", \"59\", \"6\", \"60\", \"61\", \"62\", \"63\", \"64\", \"65\", \"66\", \"67\", \"68\", \"69\", \"7\", \"70\", \"71\", \"72\", \"73\", \"74\", \"75\", \"76\", \"77\", \"78\", \"79\", \"8\", \"80\", \"81\", \"82\", \"83\", \"84\", \"85\", \"86\", \"87\", \"88\", \"89\", \"9\", \"90\", \"91\", \"92\", \"93\", \"94\", \"95\", \"96\", \"97\", \"98\", \"99\"}"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       empt {}"}));

  testing::remove_file(filename);
}