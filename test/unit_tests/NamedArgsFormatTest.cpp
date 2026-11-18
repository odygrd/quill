#include "doctest/doctest.h"

#include "quill/backend/NamedArgsFormat.h"

#include <string>
#include <utility>
#include <vector>

TEST_SUITE_BEGIN("NamedArgsFormat");

using namespace quill::detail;

using NamedArgs = std::vector<std::pair<std::string, std::string>>;

/***/
TEST_CASE("process_named_args_format_message_replaces_names")
{
  auto const [fmt_str, keys] = process_named_args_format_message("Hello {name}, {value:.2f} and {{escaped}}");

  REQUIRE_EQ(fmt_str, "Hello {}, {:.2f} and {{escaped}}");
  REQUIRE_EQ(keys, NamedArgs{{"name", ""}, {"value", ":.2f"}});
}

/***/
TEST_CASE("process_named_args_format_message_makes_duplicate_names_unique")
{
  auto const [fmt_str, keys] = process_named_args_format_message("{x} {x} {x}");

  REQUIRE_EQ(fmt_str, "{} {} {}");
  REQUIRE_EQ(keys, NamedArgs{{"x", ""}, {"x_1", ""}, {"x_2", ""}});
}

/***/
TEST_CASE("process_named_args_format_message_counts_nested_fields")
{
  // The nested width consumes an argument, which gets a generated name
  auto const [fmt_str, keys] = process_named_args_format_message("{value:>{width}} end");

  REQUIRE_EQ(fmt_str, "{:>{}} end");
  REQUIRE_EQ(keys, NamedArgs{{"value", ":>{}"}, {"_1", ""}});
}

/***/
TEST_CASE("process_named_args_format_message_keeps_unterminated_placeholder")
{
  auto const [fmt_str, keys] = process_named_args_format_message("text {unterminated");

  REQUIRE_EQ(fmt_str, "text {unterminated");
  REQUIRE(keys.empty());
}

TEST_SUITE_END();
