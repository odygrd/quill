#include "doctest/doctest.h"

#include "quill/detail/misc/TypeTraitsSerializable.h"
#include <array>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <functional>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

TEST_SUITE_BEGIN("TypeTraitsSerializable");

using namespace quill::detail;

struct Trivial
{
  int a;
};

struct NonTrivial
{
public:
  explicit NonTrivial(std::string x) : x(std::move(x)){};

private:
  std::string x;
};

enum Enum
{
  One,
  Two
};

enum class EnumClass
{
  Three,
  Four
};

TEST_CASE("is_serializable")
{
  static_assert(is_serializable<std::string>::value, "_");
  static_assert(is_serializable<uint32_t>::value, "_");
  static_assert(is_serializable<float>::value, "_");
  static_assert(is_serializable<char>::value, "_");
  static_assert(is_serializable<wchar_t>::value, "_");
  static_assert(is_serializable<bool>::value, "_");
  static_assert(is_serializable<void*>::value, "_");
  static_assert(is_serializable<uintptr_t>::value, "_");
  static_assert(is_serializable<intptr_t>::value, "_");
  static_assert(is_serializable<size_t>::value, "_");

  static_assert(is_serializable<char[10]>::value, "_");
  static_assert(is_serializable<char const*>::value, "_");
  static_assert(is_serializable<char*>::value, "_");
  static_assert(is_serializable<char const[10]>::value, "_");
  static_assert(is_serializable<decltype("string")>::value, "_");

  static_assert(!is_serializable<EnumClass>::value, "_");
  static_assert(!is_serializable<Enum>::value, "_");

  static_assert(!is_serializable<NonTrivial>::value, "_");
  static_assert(!is_serializable<Trivial>::value, "_");
  static_assert(!is_serializable<std::vector<int>>::value, "_");
  static_assert(!is_serializable<std::pair<int, int>>::value, "_");
  static_assert(!is_serializable<std::array<int, 10>>::value, "_");

  static_assert(!is_serializable<int[10]>::value, "_");
  static_assert(!is_serializable<wchar_t[10]>::value, "_");
  static_assert(!is_serializable<wchar_t const[10]>::value, "_");
  static_assert(!is_serializable<std::wstring>::value, "_");
  static_assert(!is_serializable<int*>::value, "_");
  static_assert(!is_serializable<char16_t>::value, "_");
  static_assert(!is_serializable<char32_t>::value, "_");
  static_assert(!is_serializable<int const*>::value, "_");
}

TEST_CASE("is_all_serializable")
{
  static_assert(is_all_serializable<uint32_t, uint64_t, std::string>::value, "_");
  static_assert(!is_all_serializable<uint32_t, NonTrivial>::value, "_");
  static_assert(!is_all_serializable<uint32_t, int[8]>::value, "_");
}

TEST_SUITE_END();