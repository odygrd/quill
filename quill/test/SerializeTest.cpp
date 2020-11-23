#include "doctest/doctest.h"

#include "misc/DocTestExtensions.h"
#include "quill/detail/serialize/Serialize.h"
#include <string>

TEST_SUITE_BEGIN("Serialize");

using namespace quill::detail;
using namespace quill;

struct TestClass
{
};

enum Enum : uint64_t
{
  One,
  Two
};

enum class EnumClass : char
{
  Three,
  Four
};

TEST_CASE("argument_size")
{
  REQUIRE_EQ(argument_size(true), sizeof(bool));

  REQUIRE_EQ(argument_size(uint8_t{10}), sizeof(uint8_t));
  REQUIRE_EQ(argument_size(uint16_t{10}), sizeof(uint16_t));
  REQUIRE_EQ(argument_size(uint32_t{10}), sizeof(uint32_t));
  REQUIRE_EQ(argument_size(uint64_t{10}), sizeof(uint64_t));

  REQUIRE_EQ(argument_size(int8_t{10}), sizeof(int8_t));
  REQUIRE_EQ(argument_size(int16_t{10}), sizeof(int16_t));
  REQUIRE_EQ(argument_size(int32_t{10}), sizeof(int32_t));
  REQUIRE_EQ(argument_size(int64_t{10}), sizeof(int64_t));

  REQUIRE_EQ(argument_size(size_t{10}), sizeof(size_t));
  REQUIRE_EQ(argument_size(uintptr_t{10}), sizeof(uintptr_t));
  REQUIRE_EQ(argument_size(intptr_t{10}), sizeof(intptr_t));

  REQUIRE_EQ(argument_size(double{10}), sizeof(double));
  REQUIRE_EQ(argument_size<long double>(double{10.0}), sizeof(long double));
  REQUIRE_EQ(argument_size(float{10}), sizeof(float));

  REQUIRE_EQ(argument_size(char{'z'}), sizeof(char));
  REQUIRE_EQ(argument_size<char16_t>('z'), sizeof(char16_t));
  REQUIRE_EQ(argument_size<char32_t>('z'), sizeof(char32_t));
  REQUIRE_EQ(argument_size<wchar_t>(L'z'), sizeof(wchar_t));

  REQUIRE_EQ(argument_size("test"), 5);

  // Strings
  std::string s1 = "123456789123456789123456789123456789123456789";
  REQUIRE_EQ(argument_size(s1), 46);
  REQUIRE_EQ(argument_size(std::move(s1)), 46);

  // Char Arrays
  REQUIRE_EQ(argument_size("123456789123456789123456789123456789123456789"), 46);

  // enums
  EnumClass ec;
  REQUIRE_EQ(argument_size(ec), sizeof(char));
  Enum en;
  REQUIRE_EQ(argument_size(en), sizeof(uint64_t));

  // pointers
  void* ptr{nullptr};
  REQUIRE_EQ(argument_size(ptr), sizeof(ptr));

#if defined(QUILL_USE_STRING_VIEW)
  auto sv1 = std::string_view{"123456789123456789123456789123456789123456789"};
  auto sv2 = std::wstring_view{L"123456789123456789123456789123456789123456789"};
  REQUIRE_EQ(argument_size(sv1), 46);
  REQUIRE_EQ(argument_size(sv2), 46);
#endif
}

TEST_SUITE_END();