#include "doctest/doctest.h"

#include "misc/DocTestExtensions.h"
#include "quill/detail/serialize/TypeDescriptor.h"
#include <cstdio>
#include <string>

TEST_SUITE_BEGIN("TypeDescriptor");

using namespace quill::detail;
using namespace quill;

enum Enum : long int
{
  One,
  Two
};

enum class EnumClass : char
{
  Three,
  Four
};

TEST_CASE("type_descriptor_string")
{
  std::string const s1 = type_descriptor_string<char, std::string, unsigned long, double>();
  REQUIRE_STREQ(s1.c_str(), "%C%S%UIL%D");

  std::string const s2 = type_descriptor_string<unsigned int, EnumClass, void*, double>();
  REQUIRE_STREQ(s2.c_str(), "%UI%C%P%D");

  std::string const s3 = type_descriptor_string<char const[32], EnumClass, Enum, double>();
  REQUIRE_STREQ(s3.c_str(), "%SC%C%IL%D");

  std::string const s4 = type_descriptor_string<>();
  REQUIRE_STREQ(s4.c_str(), "");

  std::string const s5 = type_descriptor_string<double>();
  REQUIRE_STREQ(s5.c_str(), "%D");
}
TEST_SUITE_END();