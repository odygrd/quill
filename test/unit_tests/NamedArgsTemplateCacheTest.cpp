#include "doctest/doctest.h"

#include "quill/backend/NamedArgsTemplateCache.h"

#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

TEST_SUITE_BEGIN("NamedArgsTemplateCache");

using namespace quill::detail;

/***/
TEST_CASE("named_args_template_cache_rewrites_each_format_string_once")
{
  NamedArgsTemplateCache cache;
  char const* format = "Iteration: {iteration} int: {value}";
  char const* other_format = "other {value:>4}";

  NamedArgsTemplate const& named_args_template = cache.get(format);
  REQUIRE_EQ(named_args_template.format(), "Iteration: {} int: {}");
  REQUIRE_EQ(named_args_template.arg_names().size(), 2);

  // The same address returns the same entry, another one has its own
  REQUIRE_EQ(&cache.get(format), &named_args_template);
  REQUIRE_NE(&cache.get(other_format), &named_args_template);
  REQUIRE_EQ(cache.get(other_format).format(), "other {:>4}");
  REQUIRE_EQ(&cache.get(format), &named_args_template);
}

/***/
TEST_CASE("named_args_template_cache_holds_many_format_strings")
{
  NamedArgsTemplateCache cache;
  std::vector<std::string> formats;

  for (size_t i = 0; i < 500; ++i)
  {
    formats.push_back("format " + std::to_string(i) + " {value}");
  }

  for (size_t round = 0; round < 3; ++round)
  {
    for (size_t i = 0; i < formats.size(); ++i)
    {
      // A different order in each round
      size_t const index = (round == 1) ? (formats.size() - 1 - i) : i;

      REQUIRE_EQ(cache.get(formats[index].c_str()).format(),
                 "format " + std::to_string(index) + " {}");
    }
  }
}

/***/
TEST_CASE("named_args_template_cache_rewrites_a_new_string_at_a_cached_address")
{
  // A buffer that is reused for another format string, as when a shared library is unloaded and
  // a different one is loaded at the same address
  NamedArgsTemplateCache cache;
  char format[32];

  std::snprintf(format, sizeof(format), "%s", "{first}");
  REQUIRE_EQ(cache.get(format).arg_names()[0].first, "first");

  std::snprintf(format, sizeof(format), "%s", "{second:>3} longer");
  NamedArgsTemplate const& named_args_template = cache.get(format);
  REQUIRE_EQ(named_args_template.format(), "{:>3} longer");
  REQUIRE_EQ(named_args_template.arg_names()[0].first, "second");

  // A shorter string that is a prefix of the cached one
  std::snprintf(format, sizeof(format), "%s", "{second:>3}");
  REQUIRE_EQ(cache.get(format).format(), "{:>3}");
}

/***/
TEST_CASE("named_args_template_cache_clear")
{
  NamedArgsTemplateCache cache;

  REQUIRE_EQ(cache.get("value {value}").format(), "value {}");

  cache.clear();

  REQUIRE_EQ(cache.get("value {value}").format(), "value {}");
}

TEST_SUITE_END();
