#include "doctest/doctest.h"

#include "misc/DocTestExtensions.h"
#include "quill/core/DynamicFormatArgStore.h"

#include "quill/bundled/fmt/format.h"

#include <cstdlib>
#include <stdexcept>
#include <string>

TEST_SUITE_BEGIN("DynamicFormatArgStore");

using namespace quill;
using namespace quill::detail;

/***/
TEST_CASE("dynamic_format_arg_store")
{
  // DynamicFormatArgStore store;
  DynamicFormatArgStore store;

  store.push_back(42);
  store.push_back(std::string_view{"abc"});
  store.push_back(1.5f);

  // c style string allocates
  store.push_back("efg");

  std::string result =
    fmtquill::vformat("{} and {} and {} and {}",
                      fmtquill::basic_format_args<fmtquill::format_context>{store.data(), store.size()});

  REQUIRE_EQ(result, std::string{"42 and abc and 1.5 and efg"});
}

namespace
{
struct PriceConvertibleToString
{
  long cents{0};

  // An implicit conversion, e.g. for a database layer, that must not replace the formatter below
  operator std::string() const { return std::to_string(cents); }
};

struct ThrowingConversionToString
{
  int value{0};

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

struct NoFormatterConvertibleToString
{
  // Without a formatter the value is still formatted through the conversion
  operator std::string() const { return "converted"; }
};

struct NonConstFormatterConvertibleToString
{
  // The formatter below takes a non-const reference, the stored value must still be formatted with it
  operator std::string() const { return "converted"; }
};
} // namespace

template <>
struct fmtquill::formatter<PriceConvertibleToString>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(PriceConvertibleToString const& price, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "${}.{:02}", price.cents / 100, price.cents % 100);
  }
};

template <>
struct fmtquill::formatter<ThrowingConversionToString>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(ThrowingConversionToString const& arg, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "value={}", arg.value);
  }
};

template <>
struct fmtquill::formatter<NonConstFormatterConvertibleToString>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(NonConstFormatterConvertibleToString&, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "formatted");
  }
};

/***/
TEST_CASE("dynamic_format_arg_store_custom_type_convertible_to_string_uses_formatter")
{
  DynamicFormatArgStore store;

  store.push_back(PriceConvertibleToString{1234});
  store.push_back(ThrowingConversionToString{7});
  store.push_back(NonConstFormatterConvertibleToString{});

  // strings are still copied into the store
  store.push_back(std::string{"plain"});

  std::string const result = fmtquill::vformat(
    "{} {} {} {}", fmtquill::basic_format_args<fmtquill::format_context>{store.data(), store.size()});

  REQUIRE_EQ(result, std::string{"$12.34 value=7 formatted plain"});
}

/***/
TEST_CASE("dynamic_format_arg_store_type_without_formatter_uses_conversion_to_string")
{
  DynamicFormatArgStore store;

  store.push_back(NoFormatterConvertibleToString{});

  std::string const result = fmtquill::vformat(
    "{}", fmtquill::basic_format_args<fmtquill::format_context>{store.data(), store.size()});

  REQUIRE_EQ(result, std::string{"converted"});
}

TEST_SUITE_END();