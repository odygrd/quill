#include "doctest/doctest.h"

#include "quill/backend/NamedArgsTemplate.h"

#include <string>

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
}

/***/
TEST_CASE("named_args_template_prepares_another_format_string")
{
  // Preparing again replaces everything that the previous format string left
  NamedArgsTemplate named_args_template;

  named_args_template.prepare("{first} and {second:>3}");
  REQUIRE_EQ(named_args_template.format(), "{} and {:>3}");
  REQUIRE_EQ(named_args_template.arg_names().size(), 2);

  named_args_template.prepare("x{value}");
  REQUIRE_EQ(named_args_template.format(), "x{}");
  REQUIRE_EQ(named_args_template.arg_names().size(), 1);
  REQUIRE_EQ(named_args_template.arg_names()[0].first, "value");
}

TEST_SUITE_END();
