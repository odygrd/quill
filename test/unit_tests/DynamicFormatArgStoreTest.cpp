#include "doctest/doctest.h"

#include "misc/DocTestExtensions.h"
#include "quill/core/DynamicFormatArgStore.h"

#include "quill/bundled/fmt/format.h"

#include <cstdlib>
#include <new>
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

/** Lives in a buffer, as the object of a DeferredFormatCodec lives in the frontend queue */
struct QueuedValue
{
  explicit QueuedValue(int in_value) : value(in_value) {}
  QueuedValue(QueuedValue const& other) : value(other.value) { ++copies; }
  ~QueuedValue() { ++destructions; }

  int value;
  static int copies;
  static int destructions;
};

int QueuedValue::copies{0};
int QueuedValue::destructions{0};
} // namespace

template <>
struct fmtquill::formatter<QueuedValue>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::QueuedValue const& queued_value, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "queued {}", queued_value.value);
  }
};

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

/***/
TEST_CASE("dynamic_format_arg_store_tells_an_argument_with_its_own_formatter")
{
  DynamicFormatArgStore store;
  REQUIRE_FALSE(store.has_custom_type());

  // Built-in types, and a type that is stored as its conversion to std::string
  store.push_back(42);
  store.push_back(3.5);
  store.push_back(std::string{"plain"});
  store.push_back(NoFormatterConvertibleToString{});
  REQUIRE_FALSE(store.has_custom_type());

  store.push_back(PriceConvertibleToString{1234});
  REQUIRE(store.has_custom_type());

  store.clear();
  REQUIRE_FALSE(store.has_custom_type());
}

/***/
TEST_CASE("dynamic_format_arg_store_formats_a_queued_object_where_it_is")
{
  alignas(QueuedValue) unsigned char storage[sizeof(QueuedValue)];
  auto* object = new (static_cast<void*>(storage)) QueuedValue{7};

  DynamicFormatArgStore store;

  // The store refers to the object, so it formats the value that the object has by then
  store.defer_destruction(object);
  store.push_back_queued(*object);
  REQUIRE(store.has_custom_type());
  object->value = 8;

  std::string const result = fmtquill::vformat(
    "{}", fmtquill::basic_format_args<fmtquill::format_context>{store.data(), store.size()});

  REQUIRE_EQ(result, std::string{"queued 8"});
  REQUIRE_EQ(QueuedValue::copies, 0);

  // The record of the object was not consumed: the object stays alive
  store.clear();
  REQUIRE_FALSE(store.has_custom_type());
  REQUIRE_EQ(QueuedValue::destructions, 0);

  // The record is consumed
  store.defer_destruction(object);
  store.push_back_queued(*object);
  store.clear(true);
  REQUIRE_EQ(QueuedValue::destructions, 1);

  // Nothing is left to destroy
  store.clear(true);
  REQUIRE_EQ(QueuedValue::destructions, 1);
}

/***/
TEST_CASE("dynamic_format_arg_store_converts_a_queued_object_without_a_formatter")
{
  NoFormatterConvertibleToString object;

  DynamicFormatArgStore store;

  // The store keeps the std::string that the object converts to, as push_back() does
  store.push_back_queued(object);
  REQUIRE_FALSE(store.has_custom_type());

  std::string const result = fmtquill::vformat(
    "{}", fmtquill::basic_format_args<fmtquill::format_context>{store.data(), store.size()});

  REQUIRE_EQ(result, std::string{"converted"});
}

TEST_SUITE_END();