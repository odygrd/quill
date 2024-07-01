#include "doctest/doctest.h"

#include "misc/DocTestExtensions.h"
#include "quill/core/DynamicFormatArgStore.h"

#include "quill/bundled/fmt/format.h"

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

  std::string result = fmtquill::vformat(
    "{} and {} and {} and {}",
                      fmtquill::basic_format_args<fmtquill::format_context>{store.data(), store.size()});

  REQUIRE_EQ(result, std::string{"42 and abc and 1.5 and efg"});
}

TEST_SUITE_END();