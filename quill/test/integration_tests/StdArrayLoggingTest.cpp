#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/TriviallyCopyableCodec.h"
#include "quill/sinks/FileSink.h"
#include "quill/std/Array.h"

#include <array>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

using namespace quill;

/***/
struct CustomTypeTC
{
  CustomTypeTC(int n, double s, uint32_t a) : name(n), surname(s), age(a){};

private:
  template <typename T, typename Char, typename Enable>
  friend struct fmtquill::formatter;

  template <typename T>
  friend struct quill::TriviallyCopyableTypeCodec;

  template <typename T, size_t N>
  friend class std::array;

  CustomTypeTC() = default;

  int name;
  double surname;
  uint32_t age;
};

static_assert(std::is_trivially_copyable_v<CustomTypeTC>, "CustomTypeTC must be trivially copyable");

/***/
template <>
struct fmtquill::formatter<CustomTypeTC>
{
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(::CustomTypeTC const& custom_type, FormatContext& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "Name: {}, Surname: {}, Age: {}", custom_type.name,
                               custom_type.surname, custom_type.age);
  }
};

template <>
struct quill::Codec<CustomTypeTC> : quill::TriviallyCopyableTypeCodec<CustomTypeTC>
{
};

/***/
TEST_CASE("std_array_logging")
{
  static constexpr char const* filename = "std_array_logging.log";
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
    std::array<bool, 2> b = {true, false};
    LOG_INFO(logger, "v {}", b);

    std::array<char, 2> c = {'a', 'c'};
    LOG_INFO(logger, "c {}", c);

    std::array<short, 2> si = {-12, 10};
    LOG_INFO(logger, "si {}", si);

    std::array<int, 2> i = {-123, 1};
    LOG_INFO(logger, "i {}", i);

    std::array<long, 2> li = {9876, 1232};
    LOG_INFO(logger, "li {}", li);

    std::array<long long, 2> lli = {321, 231};
    LOG_INFO(logger, "lli {}", lli);

    std::array<unsigned short, 2> usi = {15, 2};
    LOG_INFO(logger, "usi {}", usi);

    std::array<unsigned int, 2> ui = {123, 2};
    LOG_INFO(logger, "ui {}", ui);

    std::array<unsigned long, 2> uli = {3213, 2876};
    LOG_INFO(logger, "uli {}", uli);

    std::array<unsigned long long, 2> ulli = {321, 1321};
    LOG_INFO(logger, "ulli {}", ulli);

    std::array<float, 2> f = {111.1f, 323.31f};
    LOG_INFO(logger, "f {}", f);

    std::array<double, 2> d = {12.1, 3213213.123};
    LOG_INFO(logger, "d {}", d);

    std::array<int, 2> const& cri = i;
    LOG_INFO(logger, "cri {}", cri);

    std::array<int, 2>& ci = i;
    LOG_INFO(logger, "ci {}", ci);

    std::array<std::string, 2> sa = {"test", "string"};
    LOG_INFO(logger, "sa {}", sa);

    std::array<std::string_view, 2> sva = {"test", "string_view"};
    LOG_INFO(logger, "sva {}", sva);

    std::array<char const*, 4> scva = {"c style", "string test", "test", "log"};
    LOG_INFO(logger, "scva {}", scva);

    std::array<std::array<int, 2>, 3> aai = {{{321, 123}, {444, 333}, {111, 222}}};
    LOG_INFO(logger, "aai {}", aai);

    std::array<std::array<char const*, 2>, 3> aacs = {
      {{"one", "two"}, {"three", "four"}, {"five", "six"}}};
    LOG_INFO(logger, "aacs {}", aacs);

    std::array<std::array<std::array<char const*, 2>, 2>, 3> aaacs = {
      {{{{"one", "two"}, {"three", "four"}}},
       {{{"five", "six"}, {"seven", "eight"}}},
       {{{"nine", "ten"}, {"eleven", "twelve"}}}}};
    LOG_INFO(logger, "aaacs {}", aaacs);

    std::array<std::array<std::array<std::string, 2>, 2>, 3> aaabcs = {
      {{{{"std_one", "two"}, {"three", "four"}}},
       {{{"five", "six"}, {"seven", "eight"}}},
       {{{"std_nine", "ten"}, {"eleven", "twelve"}}}}};
    LOG_INFO(logger, "aaabcs {}", aaabcs);

    std::array<std::array<std::array<std::string_view, 2>, 2>, 3> aaaccs = {
      {{{{"std_one", "two"}, {"three_std", "four"}}},
       {{{"five", "six"}, {"seven_std", "eight"}}},
       {{{"std_nine", "ten"}, {"eleven", "twelve"}}}}};
    LOG_INFO(logger, "aaaccs {}", aaaccs);

    LOG_INFO(logger, "scva {} sa {} ulli {} scva {} sa {} aaacs {} aai {}", scva, sa, ulli, scva, sa, aaacs, aai);

    std::array<int, 4> empt{};
    LOG_INFO(logger, "empt {}", empt);

    std::array<CustomTypeTC, 4> custom_type_tc = {CustomTypeTC{1, 2, 3}, CustomTypeTC{4, 5, 6},
                                                  CustomTypeTC{7, 8, 9}, CustomTypeTC{10, 11, 12}};
    LOG_INFO(logger, "custom_type_tc {}", custom_type_tc);
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       v [true, false]"}));

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
    file_contents, std::string{"LOG_INFO      " + logger_name + "       empt [0, 0, 0, 0]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       custom_type_tc [Name: 1, Surname: 2, Age: 3, Name: 4, Surname: 5, Age: 6, Name: 7, Surname: 8, Age: 9, Name: 10, Surname: 11, Age: 12]"}));

  testing::remove_file(filename);
}