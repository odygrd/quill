#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/DeferredFormatCodec.h"
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

struct TupleOnlyElement
{
  int value;
};

static_assert(!fmtquill::is_formattable<TupleOnlyElement, char>::value,
              "TupleOnlyElement must not have a standalone formatter");

template <>
struct quill::Codec<TupleOnlyElement>
{
  static size_t compute_encoded_size(detail::SizeCacheVector& cache, TupleOnlyElement const& arg)
  {
    return Codec<int>::compute_encoded_size(cache, arg.value);
  }

  static void encode(std::byte*& buffer, detail::SizeCacheVector const& cache,
                     uint32_t& cache_index, TupleOnlyElement const& arg)
  {
    Codec<int>::encode(buffer, cache, cache_index, arg.value);
  }

  static TupleOnlyElement decode_arg(std::byte*& buffer)
  {
    return TupleOnlyElement{Codec<int>::decode_arg(buffer)};
  }
};

template <>
struct fmtquill::formatter<std::tuple<TupleOnlyElement>>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(std::tuple<TupleOnlyElement> const& tuple, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "TupleOnly({})", std::get<0>(tuple).value);
  }
};

struct TupleNoDefaultType
{
  explicit TupleNoDefaultType(std::string value) : value(std::move(value)) {}

  TupleNoDefaultType(TupleNoDefaultType const&) = default;
  TupleNoDefaultType& operator=(TupleNoDefaultType const&) = default;
  TupleNoDefaultType(TupleNoDefaultType&&) = delete;
  TupleNoDefaultType& operator=(TupleNoDefaultType&&) = delete;

  std::string value;
};

template <>
struct fmtquill::formatter<TupleNoDefaultType>
{
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(::TupleNoDefaultType const& tuple_no_default_type, FormatContext& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "TupleNoDefault(value: {})", tuple_no_default_type.value);
  }
};

template <>
struct quill::Codec<TupleNoDefaultType> : quill::DeferredFormatCodec<TupleNoDefaultType>
{
};

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
    std::tuple<> empty_tuple;
    LOG_INFO(logger, "empty_tuple {}", empty_tuple);

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

    std::tuple<TupleNoDefaultType, int> no_default_tuple{TupleNoDefaultType{"no_default"}, 7};
    LOG_INFO(logger, "no_default_tuple {}", no_default_tuple);

    LOG_INFO(logger, "temp_no_default_tuple {}",
             std::tuple<TupleNoDefaultType, int>{TupleNoDefaultType{"temp_no_default"}, 8});

    // const-qualified element types must compile and decode through the underlying codecs
    std::tuple<int const, std::string const> const_elems_tuple{300, "const_elem"};
    LOG_INFO(logger, "const_elems_tuple {}", const_elems_tuple);

    // A formatter for the complete decoded tuple is sufficient even when an element cannot be
    // formatted on its own.
    std::tuple<TupleOnlyElement> tuple_only_formatter{TupleOnlyElement{400}};
    LOG_INFO(logger, "tuple_only_formatter {}", tuple_only_formatter);
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       empty_tuple ()"}));

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

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       no_default_tuple (TupleNoDefault(value: no_default), 7)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents,
    std::string{"LOG_INFO      " + logger_name +
                "       temp_no_default_tuple (TupleNoDefault(value: temp_no_default), 8)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       const_elems_tuple (300, \"const_elem\")"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       tuple_only_formatter TupleOnly(400)"}));

  testing::remove_file(filename);
}
