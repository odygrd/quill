#include "doctest/doctest.h"

#include "quill/backend/NamedArgsTemplate.h"

#include "quill/bundled/fmt/format.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

TEST_SUITE_BEGIN("NamedArgsTemplate");

using namespace quill::detail;

/***/
TEST_CASE("named_args_template_rewrites_the_format_string")
{
  NamedArgsTemplate named_args_template;
  named_args_template.prepare("{name} paid {price:.2f} for {{item}}");

  REQUIRE_EQ(named_args_template.format(), "{} paid {:.2f} for {{item}}");
  REQUIRE_EQ(named_args_template.arg_names().size(), 2);
  REQUIRE_EQ(named_args_template.arg_names()[0].first, "name");
  REQUIRE_EQ(named_args_template.arg_names()[0].second, "");
  REQUIRE_EQ(named_args_template.arg_names()[1].first, "price");
  REQUIRE_EQ(named_args_template.arg_names()[1].second, ":.2f");
  REQUIRE(named_args_template.can_build_from_values());
}

/***/
TEST_CASE("named_args_template_builds_messages_from_named_arg_values")
{
  NamedArgsTemplate named_args_template;
  named_args_template.prepare("{name} paid {price:.2f} for {{item}}, {name}");
  REQUIRE(named_args_template.can_build_from_values());

  std::vector<std::pair<std::string, std::string>> const values{
    {"name", "alice"}, {"price", "9.50"}, {"name_1", "bob"}};

  fmtquill::memory_buffer buffer;
  named_args_template.build_from_values(buffer, values);
  REQUIRE_EQ(std::string{buffer.data(), buffer.size()}, "alice paid 9.50 for {item}, bob");
}

/***/
TEST_CASE("named_args_template_builds_the_message_that_formatting_the_arguments_gives")
{
  int const count{42};
  double const price{3.14159};
  std::string_view const side{"buy"};

  auto const store = fmtquill::make_format_args(count, price, side);
  fmtquill::format_args const args{store};

  for (char const* format :
       {"{count} {price:.2f} {side:>6}", "[{count:#x}] {price:e} {{{side}}}",
        "{count}{price}{side}", "{count:+} and {price:10.3f} and {side:.2}",
        "no text after {count} {price} {side}", "}} {count} {{ {price} }}{{ {side}"})
  {
    NamedArgsTemplate named_args_template;
    named_args_template.prepare(format);
    REQUIRE(named_args_template.can_build_from_values());

    // Each value formatted on its own with the format spec of its named argument, as the backend does
    std::vector<std::pair<std::string, std::string>> values;

    for (size_t i = 0; i < named_args_template.arg_names().size(); ++i)
    {
      auto const arg = args.get(static_cast<int>(i));
      std::string const field = "{" + named_args_template.arg_names()[i].second + "}";
      values.emplace_back(named_args_template.arg_names()[i].first,
                          fmtquill::vformat(field, fmtquill::format_args{&arg, 1}));
    }

    fmtquill::memory_buffer built;
    named_args_template.build_from_values(built, values);

    REQUIRE_EQ(std::string{built.data(), built.size()}, fmtquill::vformat(named_args_template.format(), args));
  }
}

/***/
TEST_CASE("named_args_template_leaves_other_syntax_to_fmt")
{
  NamedArgsTemplate named_args_template;

  // A dynamic width consumes a second argument
  named_args_template.prepare("{name:>{width}}");
  REQUIRE_EQ(named_args_template.format(), "{:>{}}");
  REQUIRE_FALSE(named_args_template.can_build_from_values());

  // fmt reports the unmatched brace
  named_args_template.prepare("{name} }");
  REQUIRE_FALSE(named_args_template.can_build_from_values());

  named_args_template.prepare("{name} {unterminated");
  REQUIRE_FALSE(named_args_template.can_build_from_values());
}

/***/
TEST_CASE("named_args_template_leaves_a_format_spec_with_braces_to_fmt")
{
  int const first{8};
  double const second{-746.48};

  auto const store = fmtquill::make_format_args(first, second);
  fmtquill::format_args const args{store};

  // Unbalanced braces in a field leave some of them in the format spec of its named argument, so
  // the field in the format string is not the text that the value is formatted with
  for (char const* format :
       {"{a{b{c:-}}}", "{a{b{c:-}x}}", "{a{-{: }{}}}", "v={a{b{c:>6}}} and {price:.2f}"})
  {
    NamedArgsTemplate named_args_template;
    named_args_template.prepare(format);

    REQUIRE_FALSE(named_args_template.can_build_from_values());
  }

  // Whenever the message can be built from the values, each field is "{" + format spec + "}"
  for (char const* format : {"{a:-} {b:.1f}", "{a}{{}}{b:>9.2f}", "{{{a:#x}}}", "{a} {b} }}"})
  {
    NamedArgsTemplate named_args_template;
    named_args_template.prepare(format);
    REQUIRE(named_args_template.can_build_from_values());

    std::vector<std::pair<std::string, std::string>> values;

    for (size_t i = 0; i < named_args_template.arg_names().size(); ++i)
    {
      auto const arg = args.get(static_cast<int>(i));
      std::string const field = "{" + named_args_template.arg_names()[i].second + "}";
      values.emplace_back(named_args_template.arg_names()[i].first,
                          fmtquill::vformat(field, fmtquill::format_args{&arg, 1}));
    }

    fmtquill::memory_buffer built;
    named_args_template.build_from_values(built, values);

    REQUIRE_EQ(std::string{built.data(), built.size()}, fmtquill::vformat(named_args_template.format(), args));
  }
}

/***/
TEST_CASE("named_args_template_prepares_another_format_string")
{
  // Preparing again replaces everything that the previous format string left
  NamedArgsTemplate named_args_template;

  named_args_template.prepare("{first} and {second:>3}");
  REQUIRE_EQ(named_args_template.format(), "{} and {:>3}");
  REQUIRE_EQ(named_args_template.arg_names().size(), 2);
  REQUIRE(named_args_template.can_build_from_values());

  named_args_template.prepare("{only:>{width}}");
  REQUIRE_EQ(named_args_template.arg_names()[0].first, "only");
  REQUIRE_FALSE(named_args_template.can_build_from_values());

  named_args_template.prepare("x{value}");
  REQUIRE_EQ(named_args_template.format(), "x{}");
  REQUIRE_EQ(named_args_template.arg_names().size(), 1);
  REQUIRE(named_args_template.can_build_from_values());

  std::vector<std::pair<std::string, std::string>> const values{{"value", "3"}};
  fmtquill::memory_buffer buffer;
  named_args_template.build_from_values(buffer, values);
  REQUIRE_EQ(std::string{buffer.data(), buffer.size()}, "x3");
}

TEST_SUITE_END();
