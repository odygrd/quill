#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/DeferredFormatCodec.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include "quill/std/Pair.h"

#include <array>
#include <cstdio>
#include <map>
#include <string>
#include <string_view>
#include <vector>

using namespace quill;

struct PairNoDefaultType
{
  explicit PairNoDefaultType(std::string value) : value(std::move(value)) {}

  PairNoDefaultType(PairNoDefaultType const&) = default;
  PairNoDefaultType& operator=(PairNoDefaultType const&) = default;
  PairNoDefaultType(PairNoDefaultType&&) = default;
  PairNoDefaultType& operator=(PairNoDefaultType&&) = default;

  std::string value;
};

template <>
struct fmtquill::formatter<PairNoDefaultType>
{
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(::PairNoDefaultType const& pair_no_default_type, FormatContext& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "PairNoDefault(value: {})", pair_no_default_type.value);
  }
};

template <>
struct quill::Codec<PairNoDefaultType> : quill::DeferredFormatCodec<PairNoDefaultType>
{
};

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

    std::pair<std::pair<std::string, std::string_view>, std::pair<char const*, uint32_t>> cp = {
      {"pair", "testing"}, {"first", 2}};
    LOG_INFO(logger, "cp {}", cp);

    std::pair<int, double> empt;
    LOG_INFO(logger, "empt {}", empt);

    // Test rvalue references with pairs
    std::pair<int, std::string> rvalue_pair = {100, "hundred"};
    LOG_INFO(logger, "rvalue_pair {}", std::move(rvalue_pair));

    // Test with temporary pair
    LOG_INFO(logger, "temp_pair {}", std::pair<int, std::string>{200, "twohundred"});

    std::pair<int, PairNoDefaultType> no_default_pair{10, PairNoDefaultType{"no_default"}};
    LOG_INFO(logger, "no_default_pair {}", no_default_pair);

    LOG_INFO(logger, "temp_no_default_pair {}",
             std::pair<int, PairNoDefaultType>{20, PairNoDefaultType{"temp_no_default"}});

    // std::map's value_type is std::pair<Key const, T>; logging a dereferenced map iterator
    // must compile and decode through the underlying element codecs
    std::map<std::string, int> map_value{{"map_key", 42}};
    LOG_INFO(logger, "map_value_type {}", *map_value.begin());
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

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       rvalue_pair (100, \"hundred\")"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       temp_pair (200, \"twohundred\")"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       no_default_pair (10, PairNoDefault(value: no_default))"}));

  REQUIRE(quill::testing::file_contains(
    file_contents,
    std::string{"LOG_INFO      " + logger_name +
                "       temp_no_default_pair (20, PairNoDefault(value: temp_no_default))"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       map_value_type (\"map_key\", 42)"}));

  testing::remove_file(filename);
}
