#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"
#include "quill/std/Deque.h"

#include <cstdio>
#include <deque>
#include <string>
#include <string_view>

using namespace quill;

/***/
TEST_CASE("std_deque_logging")
{
  static constexpr char const* filename = "std_deque_logging.log";
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
    std::deque<char> c = {'a', 'c'};
    LOG_INFO(logger, "c {}", c);

    std::deque<short> si = {-12, 10};
    LOG_INFO(logger, "si {}", si);

    std::deque<int> i = {-123, 1};
    LOG_INFO(logger, "i {}", i);

    std::deque<long> li = {9876, 1232};
    LOG_INFO(logger, "li {}", li);

    std::deque<long long> lli = {321, 231};
    LOG_INFO(logger, "lli {}", lli);

    std::deque<unsigned short> usi = {15, 2};
    LOG_INFO(logger, "usi {}", usi);

    std::deque<unsigned int> ui = {123, 2};
    LOG_INFO(logger, "ui {}", ui);

    std::deque<unsigned long> uli = {3213, 2876};
    LOG_INFO(logger, "uli {}", uli);

    std::deque<unsigned long long> ulli = {321, 1321};
    LOG_INFO(logger, "ulli {}", ulli);

    std::deque<float> f = {111.1f, 323.31f};
    LOG_INFO(logger, "f {}", f);

    std::deque<double> d = {12.1, 3213213.123};
    LOG_INFO(logger, "d {}", d);

    std::deque<int> const& cri = i;
    LOG_INFO(logger, "cri {}", cri);

    std::deque<int>& ci = i;
    LOG_INFO(logger, "ci {}", ci);

    std::deque<std::string> sa = {"test", "string"};
    LOG_INFO(logger, "sa {}", sa);

    std::deque<std::string_view> sva = {"test", "string_view"};
    LOG_INFO(logger, "sva {}", sva);

    std::deque<char const*> scva = {"c style", "string test", "test", "log"};
    LOG_INFO(logger, "scva {}", scva);

    std::deque<std::deque<int>> aai = {{{321, 123}, {444, 333}, {111, 222}}};
    LOG_INFO(logger, "aai {}", aai);

    std::deque<std::deque<char const*>> aacs = {
      {{"one", "two"}, {"three", "four"}, {"five", "six"}}};
    LOG_INFO(logger, "aacs {}", aacs);

    std::deque<std::deque<std::deque<char const*>>> aaacs = {
      {{{{"one", "two"}, {"three", "four"}}},
       {{{"five", "six"}, {"seven", "eight"}}},
       {{{"nine", "ten"}, {"eleven", "twelve"}}}}};
    LOG_INFO(logger, "aaacs {}", aaacs);

    {
      std::deque<std::deque<std::deque<std::string>>> aaabcs;

      // First outer deque
      std::deque<std::deque<std::string>> first_outer = {{"std_one", "two"}, {"three", "four"}};

      // Second outer deque
      std::deque<std::deque<std::string>> second_outer = {{"five", "six"}, {"seven", "eight"}};

      // Third outer deque
      std::deque<std::deque<std::string>> third_outer = {{"std_nine", "ten"}, {"eleven", "twelve"}};

      aaabcs.push_back(first_outer);
      aaabcs.push_back(second_outer);
      aaabcs.push_back(third_outer);

      LOG_INFO(logger, "aaabcs {}", aaabcs);
    }

    {
      std::deque<std::deque<std::deque<std::string>>> aaaccs;

      // First outer deque
      std::deque<std::deque<std::string>> first_outer = {{"std_one", "two"}, {"three_std", "four"}};

      // Second outer deque
      std::deque<std::deque<std::string>> second_outer = {{"five", "six"}, {"seven_std", "eight"}};

      // Third outer deque
      std::deque<std::deque<std::string>> third_outer = {{"std_nine", "ten"}, {"eleven", "twelve"}};

      aaaccs.push_back(first_outer);
      aaaccs.push_back(second_outer);
      aaaccs.push_back(third_outer);

      LOG_INFO(logger, "aaaccs {}", aaaccs);
    }

    LOG_INFO(logger, "scva {} sa {} ulli {} scva {} sa {} aaacs {} aai {}", scva, sa, ulli, scva, sa, aaacs, aai);

    std::deque<int> loopv;
    for (int iter = 0; iter < 100; ++iter)
    {
      loopv.push_back(iter);
    }

    LOG_INFO(logger, "loopv {}", loopv);

    std::deque<std::string> loopsv;
    for (int iter = 0; iter < 100; ++iter)
    {
      loopsv.push_back(std::to_string(iter));
    }

    LOG_INFO(logger, "loopsv {}", loopsv);

    std::deque<int> empt;
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
    file_contents, std::string{"LOG_INFO      " + logger_name + "       aai [[321, 123], [444, 333], [111, 222]]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents,
    std::string{"LOG_INFO      " + logger_name +
                "       aacs [[\"one\", \"two\"], [\"three\", \"four\"], [\"five\", \"six\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       aaabcs [[[\"std_one\", \"two\"], [\"three\", \"four\"]], [[\"five\", \"six\"], [\"seven\", \"eight\"]], [[\"std_nine\", \"ten\"], [\"eleven\", \"twelve\"]]]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       aaaccs [[[\"std_one\", \"two\"], [\"three_std\", \"four\"]], [[\"five\", \"six\"], [\"seven_std\", \"eight\"]], [[\"std_nine\", \"ten\"], [\"eleven\", \"twelve\"]]]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       aaacs [[[\"one\", \"two\"], [\"three\", \"four\"]], [[\"five\", \"six\"], [\"seven\", \"eight\"]], [[\"nine\", \"ten\"], [\"eleven\", \"twelve\"]]]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       scva [\"c style\", \"string test\", \"test\", \"log\"] sa [\"test\", \"string\"] ulli [321, 1321] scva [\"c style\", \"string test\", \"test\", \"log\"] sa [\"test\", \"string\"] aaacs [[[\"one\", \"two\"], [\"three\", \"four\"]], [[\"five\", \"six\"], [\"seven\", \"eight\"]], [[\"nine\", \"ten\"], [\"eleven\", \"twelve\"]]] aai [[321, 123], [444, 333], [111, 222]]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       loopv [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       loopsv [\"0\", \"1\", \"2\", \"3\", \"4\", \"5\", \"6\", \"7\", \"8\", \"9\", \"10\", \"11\", \"12\", \"13\", \"14\", \"15\", \"16\", \"17\", \"18\", \"19\", \"20\", \"21\", \"22\", \"23\", \"24\", \"25\", \"26\", \"27\", \"28\", \"29\", \"30\", \"31\", \"32\", \"33\", \"34\", \"35\", \"36\", \"37\", \"38\", \"39\", \"40\", \"41\", \"42\", \"43\", \"44\", \"45\", \"46\", \"47\", \"48\", \"49\", \"50\", \"51\", \"52\", \"53\", \"54\", \"55\", \"56\", \"57\", \"58\", \"59\", \"60\", \"61\", \"62\", \"63\", \"64\", \"65\", \"66\", \"67\", \"68\", \"69\", \"70\", \"71\", \"72\", \"73\", \"74\", \"75\", \"76\", \"77\", \"78\", \"79\", \"80\", \"81\", \"82\", \"83\", \"84\", \"85\", \"86\", \"87\", \"88\", \"89\", \"90\", \"91\", \"92\", \"93\", \"94\", \"95\", \"96\", \"97\", \"98\", \"99\"]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       empt []"}));

  testing::remove_file(filename);
}