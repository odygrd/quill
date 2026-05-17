#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/DeferredFormatCodec.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include "quill/std/Variant.h"
#include "quill/std/Vector.h"

#include <cstdio>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

using namespace quill;

namespace
{
struct VariantThrowOnConstruction
{
  explicit VariantThrowOnConstruction(int value) : value(value) {}

  VariantThrowOnConstruction(VariantThrowOnConstruction const&) = default;

  VariantThrowOnConstruction(VariantThrowOnConstruction&&)
  {
    throw std::runtime_error{"variant move failure"};
  }

  VariantThrowOnConstruction& operator=(VariantThrowOnConstruction const&) = default;
  VariantThrowOnConstruction& operator=(VariantThrowOnConstruction&&) = default;

  int value;
};
} // namespace

template <>
struct fmtquill::formatter<VariantThrowOnConstruction>
{
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(::VariantThrowOnConstruction const& value, FormatContext& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "VariantThrowOnConstruction({})", value.value);
  }
};

template <>
struct quill::Codec<VariantThrowOnConstruction> : quill::DeferredFormatCodec<VariantThrowOnConstruction>
{
};

/***/
TEST_CASE("std_variant_logging")
{
  static constexpr char const* filename = "std_variant_logging.log";
  static std::string const logger_name = "std_variant_logger";

  Backend::start();

  Frontend::preallocate();

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

  std::variant<int, std::string> int_variant{42};
  std::variant<int, std::string> string_variant{std::string{"variant_string"}};
  std::variant<int, std::string> zero_int_variant{0};
  std::variant<int, std::string> empty_string_variant{std::string{}};
  std::variant<std::monostate, int> monostate_variant{std::monostate{}};
  std::variant<std::monostate, int> monostate_with_int{123};
  std::vector<std::variant<int, std::string>> nested_variants;
  nested_variants.emplace_back(7);
  nested_variants.emplace_back(std::string{"nested"});

  LOG_INFO(logger, "int_variant {}", int_variant);
  LOG_INFO(logger, "string_variant {}", string_variant);
  LOG_INFO(logger, "zero_int_variant {}", zero_int_variant);
  LOG_INFO(logger, "empty_string_variant {}", empty_string_variant);
  LOG_INFO(logger, "monostate_variant {}", monostate_variant);
  LOG_INFO(logger, "monostate_with_int {}", monostate_with_int);
  LOG_INFO(logger, "temp_variant {}", std::variant<int, std::string>{std::string{"temporary_variant"}});
  LOG_INFO(logger, "nested_variants {}", nested_variants);

#if !defined(QUILL_NO_EXCEPTIONS)
  std::variant<int, VariantThrowOnConstruction> valueless_variant{123};

  try
  {
    valueless_variant = VariantThrowOnConstruction{11};
  }
  catch (std::runtime_error const&)
  {
  }

  REQUIRE(valueless_variant.valueless_by_exception());
  LOG_INFO(logger, "valueless_variant {}", valueless_variant);
#endif

  logger->flush_log();
  Frontend::remove_logger(logger);
  Backend::stop();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE(quill::testing::file_contains(
    file_contents, logger_name + " int_variant " + fmtquill::format("{}", int_variant)));

  REQUIRE(quill::testing::file_contains(
    file_contents, logger_name + " string_variant " + fmtquill::format("{}", string_variant)));

  REQUIRE(quill::testing::file_contains(
    file_contents, logger_name + " zero_int_variant " + fmtquill::format("{}", zero_int_variant)));

  REQUIRE(quill::testing::file_contains(
    file_contents, logger_name + " empty_string_variant " + fmtquill::format("{}", empty_string_variant)));

  REQUIRE(quill::testing::file_contains(
    file_contents, logger_name + " monostate_variant " + fmtquill::format("{}", monostate_variant)));

  REQUIRE(quill::testing::file_contains(
    file_contents, logger_name + " monostate_with_int " + fmtquill::format("{}", monostate_with_int)));

  REQUIRE(quill::testing::file_contains(
    file_contents,
    logger_name + " temp_variant " +
      fmtquill::format("{}", std::variant<int, std::string>{std::string{"temporary_variant"}})));

  REQUIRE(quill::testing::file_contains(
    file_contents, logger_name + " nested_variants " + fmtquill::format("{}", nested_variants)));

#if !defined(QUILL_NO_EXCEPTIONS)
  REQUIRE(quill::testing::file_contains(
    file_contents, logger_name + " valueless_variant variant(valueless by exception)"));
#endif

  testing::remove_file(filename);
}
