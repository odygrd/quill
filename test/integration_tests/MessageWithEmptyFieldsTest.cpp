#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/DirectFormatCodec.h"
#include "quill/Frontend.h"
#include "quill/LogFunctions.h"
#include "quill/LogMacros.h"
#include "quill/bundled/fmt/format.h"
#include "quill/sinks/FileSink.h"
#include "quill/std/Vector.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>
#include <vector>

using namespace quill;

struct EmptyFieldsPoint
{
  int x;
  int y;
};

template <>
struct fmtquill::formatter<EmptyFieldsPoint>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(EmptyFieldsPoint const& point, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "({}, {})", point.x, point.y);
  }
};

template <>
struct quill::Codec<EmptyFieldsPoint> : quill::DirectFormatCodec<EmptyFieldsPoint>
{
};

/***/
TEST_CASE("message_with_empty_fields")
{
  static constexpr char const* filename = "message_with_empty_fields.log";

  Backend::start();

  auto file_sink = Frontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger(
    "logger_message_with_empty_fields", std::move(file_sink),
    PatternFormatterOptions{"%(log_level) %(message)", "%H:%M:%S.%Qns", Timezone::GmtTime, false});

  // A message whose braces are all "{}" fields is formatted without fmt parsing it, and must be
  // formatted as fmt formats it. Messages with other braces are still formatted by fmt
  std::string const text_with_braces{"x{}y}"};
  std::vector<int> const values{1, 2, 3};

  LOG_INFO(logger, "Connected to the exchange");
  LOG_INFO(logger, "{}", 42);
  LOG_INFO(logger, "{}{}{}", 1, 'c', true);
  LOG_INFO(logger, "Text {} and {} with braces", text_with_braces, "z{{");
  LOG_INFO(logger, "Values {} {} {} {}", static_cast<void const*>(nullptr), 1.5e300, -17,
           std::numeric_limits<uint64_t>::max());
  LOG_INFO(logger, "Point {} and vector {}", EmptyFieldsPoint{1, 2}, values);
  LOG_INFO(logger, "Escaped {{braces}} and }} with {}", 1);
  LOG_INFO_LIMIT(std::chrono::nanoseconds{0}, logger, "Rate limited {} {}", 1, 2);
  LOG_INFO_LIMIT(std::chrono::nanoseconds{0}, logger, "Rate limited without arguments");

#if !defined(QUILL_NO_EXCEPTIONS)
  // The macro-free functions do not check the arguments of a format string at compile time
  quill::info(logger, "Missing argument {} {}", 1);
  LOG_INFO(logger, "Unmatched } brace");
#endif

  logger->flush_log();
  Frontend::remove_logger_blocking(logger);
  Backend::stop();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE(quill::testing::file_contains(file_contents, std::string{"INFO Connected to the exchange"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"INFO 42"}));
  REQUIRE(quill::testing::file_contains(file_contents, "INFO " + fmtquill::format("{}{}{}", 1, 'c', true)));
  REQUIRE(quill::testing::file_contains(
    file_contents,
    "INFO " + fmtquill::format("Text {} and {} with braces", text_with_braces, "z{{")));
  REQUIRE(quill::testing::file_contains(
    file_contents,
    "INFO " +
      fmtquill::format("Values {} {} {} {}", static_cast<void const*>(nullptr), 1.5e300, -17,
                       std::numeric_limits<uint64_t>::max())));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"INFO Point (1, 2) and vector [1, 2, 3]"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"INFO Escaped {braces} and } with 1"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"INFO Rate limited 1 2 (1x)"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"INFO Rate limited without arguments (1x)"}));

#if !defined(QUILL_NO_EXCEPTIONS)
  REQUIRE_EQ(file_contents.size(), 11);
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{R"(INFO [Could not format log statement. message: "Missing argument {} {}")"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{R"(error: "argument not found")"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{R"(INFO [Could not format log statement. message: "Unmatched } brace")"}));
#else
  REQUIRE_EQ(file_contents.size(), 9);
#endif

  testing::remove_file(filename);
}
