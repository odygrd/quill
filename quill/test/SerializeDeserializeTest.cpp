#include "doctest/doctest.h"

#include "misc/DocTestExtensions.h"
#include "quill/Fmt.h"
#include "quill/detail/misc/Utilities.h"
#include "quill/detail/serialize/Deserialize.h"
#include "quill/detail/serialize/Serialize.h"
#include <algorithm>
#include <cctype>
#include <string>

TEST_SUITE_BEGIN("SerializeDeserialize");

using namespace quill::detail;
using namespace quill;

struct TestClass
{
};

TEST_CASE("type_descriptor_string")
{
  std::string const s1 = type_descriptor_string<char, std::string, unsigned long, double>();
  REQUIRE_STREQ(s1.c_str(), "mqhj");

  std::string const s2 = type_descriptor_string<unsigned int, void*, double>();
  REQUIRE_STREQ(s2.c_str(), "gpj");

  std::string const s3 = type_descriptor_string<char const[32], long int>();
  REQUIRE_STREQ(s3.c_str(), "qd");

  std::string const s4 = type_descriptor_string<>();
  REQUIRE_STREQ(s4.c_str(), "");

  std::string const s5 = type_descriptor_string<double>();
  REQUIRE_STREQ(s5.c_str(), "j");
}

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

TEST_CASE("serialize_deserialize_bool")
{
  fmt::dynamic_format_arg_store<fmt::format_context> fmt_store;
  unsigned char b[256];
  unsigned char* buffer = &b[0];
  unsigned char const* cbuffer = &b[0];

  using value_t = bool;
  value_t a{true};
  serialize_arguments(buffer, a);

  // get the type_descriptor and also erase % since we are only using it to split the string
  for (char const type_descriptor : type_descriptor_string<value_t>())
  {
    auto read_size = deserialize_argument(cbuffer, fmt_store, static_cast<TypeDescriptor>(type_descriptor));
    (void)read_size;
  }

  REQUIRE_EQ(fmt::vformat("{}", fmt_store), "true");
}

TEST_CASE("serialize_deserialize_short")
{
  fmt::dynamic_format_arg_store<fmt::format_context> fmt_store;
  unsigned char b[256];
  unsigned char* buffer = &b[0];
  unsigned char const* cbuffer = &b[0];

  using value_t = short;
  value_t a{1};
  serialize_arguments(buffer, a);

  // get the type_descriptor and also erase % since we are only using it to split the string
  for (char const type_descriptor : type_descriptor_string<value_t>())
  {
    auto read_size = deserialize_argument(cbuffer, fmt_store, static_cast<TypeDescriptor>(type_descriptor));
    (void)read_size;
  }

  REQUIRE_EQ(fmt::vformat("{}", fmt_store), "1");
}

TEST_CASE("serialize_deserialize_int")
{
  fmt::dynamic_format_arg_store<fmt::format_context> fmt_store;
  unsigned char b[256];
  unsigned char* buffer = &b[0];
  unsigned char const* cbuffer = &b[0];

  using value_t = int;
  value_t a{-123};
  serialize_arguments(buffer, a);

  // get the type_descriptor and also erase % since we are only using it to split the string
  for (char const type_descriptor : type_descriptor_string<value_t>())
  {
    auto read_size = deserialize_argument(cbuffer, fmt_store, static_cast<TypeDescriptor>(type_descriptor));
    (void)read_size;
  }

  REQUIRE_EQ(fmt::vformat("{}", fmt_store), "-123");
}

TEST_CASE("serialize_deserialize_long")
{
  fmt::dynamic_format_arg_store<fmt::format_context> fmt_store;
  unsigned char b[256];
  unsigned char* buffer = &b[0];
  unsigned char const* cbuffer = &b[0];

  using value_t = long;
  value_t a{-12345};
  serialize_arguments(buffer, a);

  // get the type_descriptor and also erase % since we are only using it to split the string
  for (char const type_descriptor : type_descriptor_string<value_t>())
  {
    auto read_size = deserialize_argument(cbuffer, fmt_store, static_cast<TypeDescriptor>(type_descriptor));
    (void)read_size;
  }

  REQUIRE_EQ(fmt::vformat("{}", fmt_store), "-12345");
}

TEST_CASE("serialize_deserialize_long_long")
{
  fmt::dynamic_format_arg_store<fmt::format_context> fmt_store;
  unsigned char b[256];
  unsigned char* buffer = &b[0];
  unsigned char const* cbuffer = &b[0];

  using value_t = long long;
  value_t a{-12345678};
  serialize_arguments(buffer, a);

  // get the type_descriptor and also erase % since we are only using it to split the string
  for (char const type_descriptor : type_descriptor_string<value_t>())
  {
    auto read_size = deserialize_argument(cbuffer, fmt_store, static_cast<TypeDescriptor>(type_descriptor));
    (void)read_size;
  }

  REQUIRE_EQ(fmt::vformat("{}", fmt_store), "-12345678");
}

TEST_CASE("serialize_deserialize_unsigned_short")
{
  fmt::dynamic_format_arg_store<fmt::format_context> fmt_store;
  unsigned char b[256];
  unsigned char* buffer = &b[0];
  unsigned char const* cbuffer = &b[0];

  using value_t = unsigned short;
  value_t a{1};
  serialize_arguments(buffer, a);

  // get the type_descriptor and also erase % since we are only using it to split the string
  for (char const type_descriptor : type_descriptor_string<value_t>())
  {
    auto read_size = deserialize_argument(cbuffer, fmt_store, static_cast<TypeDescriptor>(type_descriptor));
    (void)read_size;
  }

  REQUIRE_EQ(fmt::vformat("{}", fmt_store), "1");
}

TEST_CASE("serialize_deserialize_unsigned_int")
{
  fmt::dynamic_format_arg_store<fmt::format_context> fmt_store;
  unsigned char b[256];
  unsigned char* buffer = &b[0];
  unsigned char const* cbuffer = &b[0];

  using value_t = unsigned int;
  value_t a{123};
  serialize_arguments(buffer, a);

  // get the type_descriptor and also erase % since we are only using it to split the string
  for (char const type_descriptor : type_descriptor_string<value_t>())
  {
    auto read_size = deserialize_argument(cbuffer, fmt_store, static_cast<TypeDescriptor>(type_descriptor));
    (void)read_size;
  }

  REQUIRE_EQ(fmt::vformat("{}", fmt_store), "123");
}

TEST_CASE("serialize_deserialize_unsigned_long")
{
  fmt::dynamic_format_arg_store<fmt::format_context> fmt_store;
  unsigned char b[256];
  unsigned char* buffer = &b[0];
  unsigned char const* cbuffer = &b[0];

  using value_t = unsigned long;
  value_t a{12345};
  serialize_arguments(buffer, a);

  // get the type_descriptor and also erase % since we are only using it to split the string
  for (char const type_descriptor : type_descriptor_string<value_t>())
  {
    auto read_size = deserialize_argument(cbuffer, fmt_store, static_cast<TypeDescriptor>(type_descriptor));
    (void)read_size;
  }

  REQUIRE_EQ(fmt::vformat("{}", fmt_store), "12345");
}

TEST_CASE("serialize_deserialize_unsigned_long_long")
{
  fmt::dynamic_format_arg_store<fmt::format_context> fmt_store;
  unsigned char b[256];
  unsigned char* buffer = &b[0];
  unsigned char const* cbuffer = &b[0];

  using value_t = unsigned long long;
  value_t a{12345678};
  serialize_arguments(buffer, a);

  // get the type_descriptor and also erase % since we are only using it to split the string
  for (char const type_descriptor : type_descriptor_string<value_t>())
  {
    auto read_size = deserialize_argument(cbuffer, fmt_store, static_cast<TypeDescriptor>(type_descriptor));
    (void)read_size;
  }

  REQUIRE_EQ(fmt::vformat("{}", fmt_store), "12345678");
}

TEST_CASE("serialize_deserialize_unsigned_double")
{
  fmt::dynamic_format_arg_store<fmt::format_context> fmt_store;
  unsigned char b[256];
  unsigned char* buffer = &b[0];
  unsigned char const* cbuffer = &b[0];

  using value_t = double;
  value_t a{12.34};
  serialize_arguments(buffer, a);

  // get the type_descriptor and also erase % since we are only using it to split the string
  for (char const type_descriptor : type_descriptor_string<value_t>())
  {
    auto read_size = deserialize_argument(cbuffer, fmt_store, static_cast<TypeDescriptor>(type_descriptor));
    (void)read_size;
  }

  REQUIRE_EQ(fmt::vformat("{}", fmt_store), "12.34");
}

TEST_CASE("serialize_deserialize_unsigned_long_double")
{
  fmt::dynamic_format_arg_store<fmt::format_context> fmt_store;
  unsigned char b[256];
  unsigned char* buffer = &b[0];
  unsigned char const* cbuffer = &b[0];

  using value_t = long double;
  value_t a{12.345};
  serialize_arguments(buffer, a);

  // get the type_descriptor and also erase % since we are only using it to split the string
  for (char const type_descriptor : type_descriptor_string<value_t>())
  {
    auto read_size = deserialize_argument(cbuffer, fmt_store, static_cast<TypeDescriptor>(type_descriptor));
    (void)read_size;
  }

  REQUIRE_EQ(fmt::vformat("{}", fmt_store), "12.345");
}

TEST_CASE("serialize_deserialize_unsigned_float")
{
  fmt::dynamic_format_arg_store<fmt::format_context> fmt_store;
  unsigned char b[256];
  unsigned char* buffer = &b[0];
  unsigned char const* cbuffer = &b[0];

  using value_t = float;
  value_t a{123.3f};
  serialize_arguments(buffer, a);

  // get the type_descriptor and also erase % since we are only using it to split the string
  for (char const type_descriptor : type_descriptor_string<value_t>())
  {
    auto read_size = deserialize_argument(cbuffer, fmt_store, static_cast<TypeDescriptor>(type_descriptor));
    (void)read_size;
  }

  REQUIRE_EQ(fmt::vformat("{}", fmt_store), "123.3");
}

TEST_CASE("serialize_deserialize_char")
{
  fmt::dynamic_format_arg_store<fmt::format_context> fmt_store;
  unsigned char b[256];
  unsigned char* buffer = &b[0];
  unsigned char const* cbuffer = &b[0];

  using value_t = char;
  value_t a{'a'};
  serialize_arguments(buffer, a);

  // get the type_descriptor and also erase % since we are only using it to split the string
  for (char const type_descriptor : type_descriptor_string<value_t>())
  {
    auto read_size = deserialize_argument(cbuffer, fmt_store, static_cast<TypeDescriptor>(type_descriptor));
    (void)read_size;
  }

  REQUIRE_EQ(fmt::vformat("{}", fmt_store), "a");
}

TEST_CASE("serialize_deserialize_unsigned_char")
{
  fmt::dynamic_format_arg_store<fmt::format_context> fmt_store;
  unsigned char b[256];
  unsigned char* buffer = &b[0];
  unsigned char const* cbuffer = &b[0];

  using value_t = unsigned char;
  value_t a{'b'};
  serialize_arguments(buffer, a);

  // get the type_descriptor and also erase % since we are only using it to split the string
  for (char const type_descriptor : type_descriptor_string<value_t>())
  {
    auto read_size = deserialize_argument(cbuffer, fmt_store, static_cast<TypeDescriptor>(type_descriptor));
    (void)read_size;
  }

  REQUIRE_EQ(fmt::vformat("{}", fmt_store), "98");
}

TEST_CASE("serialize_deserialize_signed_char")
{
  fmt::dynamic_format_arg_store<fmt::format_context> fmt_store;
  unsigned char b[256];
  unsigned char* buffer = &b[0];
  unsigned char const* cbuffer = &b[0];

  using value_t = char;
  value_t a{'c'};
  serialize_arguments(buffer, a);

  // get the type_descriptor and also erase % since we are only using it to split the string
  for (char const type_descriptor : type_descriptor_string<value_t>())
  {
    auto read_size = deserialize_argument(cbuffer, fmt_store, static_cast<TypeDescriptor>(type_descriptor));
    (void)read_size;
  }

  REQUIRE_EQ(fmt::vformat("{}", fmt_store), "c");
}

TEST_CASE("serialize_deserialize_signed_void_ptr")
{
  fmt::dynamic_format_arg_store<fmt::format_context> fmt_store;
  unsigned char b[256];
  unsigned char* buffer = &b[0];
  unsigned char const* cbuffer = &b[0];

  int x;
  // use stringstream to store the expected ptr value
  std::stringstream ss;
  ss << std::addressof(x);

  using value_t = void*;
  value_t a{std::addressof(x)};
  serialize_arguments(buffer, a);

  // get the type_descriptor and also erase % since we are only using it to split the string
  for (char const type_descriptor : type_descriptor_string<value_t>())
  {
    auto read_size = deserialize_argument(cbuffer, fmt_store, static_cast<TypeDescriptor>(type_descriptor));
    (void)read_size;
  }

  // on windows we get like original: 0xaff81c and for ss.str(): 00AFF81C

  // remove first 2 characters from expected string and make it all lower
  std::string expected_str = ss.str();
  expected_str.erase(expected_str.begin(), expected_str.begin() + 2);
  expected_str.erase(std::remove(expected_str.begin(), expected_str.end(), '0'), expected_str.end());
  std::transform(expected_str.begin(), expected_str.end(), expected_str.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  // actual value - make it all lower and remove the first 2 chars
  std::string actual_str = fmt::vformat("{}", fmt_store);
  actual_str.erase(actual_str.begin(), actual_str.begin() + 2);
  actual_str.erase(std::remove(actual_str.begin(), actual_str.end(), '0'), actual_str.end());
  std::transform(actual_str.begin(), actual_str.end(), actual_str.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  REQUIRE_EQ(actual_str, expected_str);
}

TEST_CASE("serialize_deserialize_string")
{
  fmt::dynamic_format_arg_store<fmt::format_context> fmt_store;
  unsigned char b[256];
  unsigned char* buffer = &b[0];
  unsigned char const* cbuffer = &b[0];

  using value_t = std::string;
  value_t a{"test serialize_deserialize_string"};
  serialize_arguments(buffer, a);

  // get the type_descriptor and also erase % since we are only using it to split the string
  for (char const type_descriptor : type_descriptor_string<value_t>())
  {
    auto read_size = deserialize_argument(cbuffer, fmt_store, static_cast<TypeDescriptor>(type_descriptor));
    (void)read_size;
  }

  REQUIRE_EQ(fmt::vformat("{}", fmt_store), "test serialize_deserialize_string");
}

TEST_CASE("serialize_deserialize_char_ptr")
{
  fmt::dynamic_format_arg_store<fmt::format_context> fmt_store;
  unsigned char b[256];
  unsigned char* buffer = &b[0];
  unsigned char const* cbuffer = &b[0];

  using value_t = char const*;
  value_t a{"test serialize_deserialize_string"};
  serialize_arguments(buffer, a);

  // get the type_descriptor and also erase % since we are only using it to split the string
  for (char const type_descriptor : type_descriptor_string<value_t>())
  {
    auto read_size = deserialize_argument(cbuffer, fmt_store, static_cast<TypeDescriptor>(type_descriptor));
    (void)read_size;
  }

  REQUIRE_EQ(fmt::vformat("{}", fmt_store), "test serialize_deserialize_string");
}

TEST_CASE("serialize_deserialize_char_array")
{
  fmt::dynamic_format_arg_store<fmt::format_context> fmt_store;
  unsigned char b[256];
  unsigned char* buffer = &b[0];
  unsigned char const* cbuffer = &b[0];

  using value_t = char const[];
  value_t a{"test serialize_deserialize_string"};
  serialize_arguments(buffer, a);

  // get the type_descriptor and also erase % since we are only using it to split the string
  for (char const type_descriptor : type_descriptor_string<value_t>())
  {
    auto read_size = deserialize_argument(cbuffer, fmt_store, static_cast<TypeDescriptor>(type_descriptor));
    (void)read_size;
  }

  REQUIRE_EQ(fmt::vformat("{}", fmt_store), "test serialize_deserialize_string");
}

TEST_CASE("serialize_deserialize_many")
{
  fmt::dynamic_format_arg_store<fmt::format_context> fmt_store;
  unsigned char b[256];
  unsigned char* buffer = &b[0];
  unsigned char const* cbuffer = &b[0];

  char const* v1{"test serialize_deserialize_string"};
  int v2{12345};
  double v3{99.32};
  std::string v4{"value"};
  float v5{12.3f};
  serialize_arguments(buffer, v1, v2, v3, v4, v5);

  // get the type_descriptor and also erase % since we are only using it to split the string
  std::string type_descriptor_s;
  construct_type_descriptor_string<char const*, int, double, std::string, float>(type_descriptor_s);

  for (char const type_descriptor : type_descriptor_s)
  {
    auto read_size = deserialize_argument(cbuffer, fmt_store, static_cast<TypeDescriptor>(type_descriptor));
    (void)read_size;
  }

  REQUIRE_EQ(fmt::vformat("{} {} {} {} {}", fmt_store),
             "test serialize_deserialize_string 12345 99.32 value 12.3");
}

#if defined(QUILL_USE_STRING_VIEW)
TEST_CASE("serialize_deserialize_string_view")
{
  fmt::dynamic_format_arg_store<fmt::format_context> fmt_store;
  unsigned char b[256];
  unsigned char* buffer = &b[0];
  unsigned char const* cbuffer = &b[0];

  using value_t = std_string_view<char>;
  value_t a{"test serialize_deserialize_string"};
  serialize_arguments(buffer, a);

  // get the type_descriptor and also erase % since we are only using it to split the string
  for (char const type_descriptor : type_descriptor_string<value_t>())
  {
    deserialize_argument(cbuffer, fmt_store, static_cast<TypeDescriptor>(type_descriptor));
  }

  REQUIRE_EQ(fmt::vformat("{}", fmt_store), "test serialize_deserialize_string");
}
#endif

TEST_SUITE_END();