#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/DeferredFormatCodec.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/bundled/fmt/format.h"
#include "quill/sinks/FileSink.h"

#include <string>
#include <utility>
#include <vector>

using namespace quill;

/** A price that is formatted with the precision given by the first argument of the log statement */
struct PriceWithPrecisionArgument
{
  double value;
};

/** Visits a format argument: the value of an int, zero for an argument of another type */
struct IntArgumentVisitor
{
  int operator()(int value) const noexcept { return value; }

  template <typename T>
  int operator()(T const&) const noexcept
  {
    return 0;
  }
};

/***/
template <>
struct fmtquill::formatter<PriceWithPrecisionArgument>
{
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(::PriceWithPrecisionArgument const& price, FormatContext& ctx) const
  {
    int const precision = ctx.arg(0).visit(IntArgumentVisitor{});

    return fmtquill::format_to(ctx.out(), "{:.{}f}", price.value, precision);
  }
};

template <>
struct quill::Codec<PriceWithPrecisionArgument> : quill::DeferredFormatCodec<PriceWithPrecisionArgument>
{
};

/***/
TEST_CASE("named_args_user_formatter_context")
{
  static constexpr char const* filename = "named_args_user_formatter_context.log";

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

  Logger* logger =
    Frontend::create_or_get_logger("named_args_user_formatter_context_logger", std::move(file_sink),
                                   quill::PatternFormatterOptions{"%(message)"});

  // The formatter of a user type can read the other arguments of the statement through its format
  // context, which fmt gives it for a field with a format spec, here an empty one. The message
  // with named fields must be the one that positional fields give
  LOG_INFO(logger, "{} {:}", 2, PriceWithPrecisionArgument{12.3456});
  LOG_INFO(logger, "{precision} {price:}", 2, PriceWithPrecisionArgument{12.3456});
  LOG_INFO(logger, "{precision} {price:} {currency}", 3, PriceWithPrecisionArgument{12.3456}, "USD");

  logger->flush_log();
  Frontend::remove_logger_blocking(logger);
  Backend::stop();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE_EQ(file_contents.size(), 3u);
  REQUIRE_EQ(file_contents[0], "2 12.35");
  REQUIRE_EQ(file_contents[1], "2 12.35");
  REQUIRE_EQ(file_contents[2], "3 12.346 USD");

  testing::remove_file(filename);
}
