#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include "quill/DeferredFormatCodec.h"
#include "quill/bundled/fmt/format.h"

#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

using namespace quill;

/***/
struct DeferredPrice
{
  long cents{0};

  // An implicit conversion, e.g. for a database layer, that must not replace the formatter below
  operator std::string() const { return std::to_string(cents); }
};

/***/
template <>
struct fmtquill::formatter<DeferredPrice>
{
  bool as_cents{false};

  constexpr auto parse(format_parse_context& ctx)
  {
    auto it = ctx.begin();

    if ((it != ctx.end()) && (*it == 'c'))
    {
      as_cents = true;
      ++it;
    }

    return it;
  }

  auto format(::DeferredPrice const& price, format_context& ctx) const
  {
    if (as_cents)
    {
      return fmtquill::format_to(ctx.out(), "{}c", price.cents);
    }

    return fmtquill::format_to(ctx.out(), "${}.{:02}", price.cents / 100, price.cents % 100);
  }
};

/***/
template <>
struct quill::Codec<DeferredPrice> : quill::DeferredFormatCodec<DeferredPrice>
{
};

/***/
struct DeferredDocument
{
  int id{0};

  // Like nlohmann::json, which converts implicitly but throws unless it holds a string
  operator std::string() const
  {
#if defined(QUILL_NO_EXCEPTIONS)
    std::abort();
#else
    throw std::logic_error{"not a string"};
#endif
  }
};

/***/
template <>
struct fmtquill::formatter<DeferredDocument>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::DeferredDocument const& document, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "Document({})", document.id);
  }
};

/***/
template <>
struct quill::Codec<DeferredDocument> : quill::DeferredFormatCodec<DeferredDocument>
{
};

/***/
struct DeferredNonConstFormatted
{
  operator std::string() const { return "converted"; }
};

/***/
template <>
struct fmtquill::formatter<DeferredNonConstFormatted>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  // A formatter that takes a non-const reference must also be used
  auto format(::DeferredNonConstFormatted&, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "formatted");
  }
};

/***/
template <>
struct quill::Codec<DeferredNonConstFormatted> : quill::DeferredFormatCodec<DeferredNonConstFormatted>
{
};

/***/
TEST_CASE("user_defined_type_deferred_format_convertible_to_string")
{
  static constexpr char const* filename =
    "user_defined_type_deferred_format_convertible_to_string.log";
  static std::string const logger_name = "logger_deferred_convertible_to_string";

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

  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));

  LOG_INFO(logger, "Price: {}", DeferredPrice{1234});
  LOG_INFO(logger, "Price in cents: {:c}", DeferredPrice{1234});
  LOG_INFO(logger, "Doc: {}", DeferredDocument{7});
  LOG_INFO(logger, "Fallback: {}", DeferredNonConstFormatted{});

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // The user formatter must be used, not the implicit conversion to std::string
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 4);
  REQUIRE(quill::testing::file_contains(file_contents, "Price: $12.34"));
  REQUIRE(quill::testing::file_contains(file_contents, "Price in cents: 1234c"));
  REQUIRE(quill::testing::file_contains(file_contents, "Doc: Document(7)"));
  REQUIRE(quill::testing::file_contains(file_contents, "Fallback: formatted"));

  testing::remove_file(filename);
}
