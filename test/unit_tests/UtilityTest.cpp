#include "doctest/doctest.h"

#include "quill/Utility.h"
#include <cstdint>
#include <cstring>

TEST_SUITE_BEGIN("Utility");

/***/
TEST_CASE("ascii_string_to_hex_uppercase")
{
  std::string buffer = "Hello World";
  std::string const result = quill::utility::to_hex(buffer.data(), buffer.length(), true);
  std::string const expected = "48 65 6C 6C 6F 20 57 6F 72 6C 64";
  REQUIRE_EQ(result, expected);
}

/***/
TEST_CASE("ascii_string_to_hex_lowercase")
{
  std::string buffer = "Hello World";
  std::string const result = quill::utility::to_hex(buffer.data(), buffer.length(), false);
  std::string const expected = "48 65 6c 6c 6f 20 57 6f 72 6c 64";
  REQUIRE_EQ(result, expected);
}

/***/
TEST_CASE("ascii_string_to_hex_default")
{
  std::string buffer = "Hello World";
  // Default should be uppercase
  std::string const result = quill::utility::to_hex(buffer.data(), buffer.length());
  std::string const expected = "48 65 6C 6C 6F 20 57 6F 72 6C 64";
  REQUIRE_EQ(result, expected);
}

/***/
TEST_CASE("longer_ascii_string_to_hex_uppercase")
{
  std::string buffer = "A longer ASCII text";
  std::string const result = quill::utility::to_hex(buffer.data(), buffer.length(), true);
  std::string const expected = "41 20 6C 6F 6E 67 65 72 20 41 53 43 49 49 20 74 65 78 74";
  REQUIRE_EQ(result, expected);
}

/***/
TEST_CASE("longer_ascii_string_to_hex_lowercase")
{
  std::string buffer = "A longer ASCII text";
  std::string const result = quill::utility::to_hex(buffer.data(), buffer.length(), false);
  std::string const expected = "41 20 6c 6f 6e 67 65 72 20 41 53 43 49 49 20 74 65 78 74";
  REQUIRE_EQ(result, expected);
}

/***/
TEST_CASE("const_string_to_hex_uppercase_and_lowercase")
{
  std::string const buffer = "A longer ASCII text";

  // Test uppercase
  std::string const result_upper = quill::utility::to_hex(buffer.data(), buffer.length(), true);
  std::string const expected_upper = "41 20 6C 6F 6E 67 65 72 20 41 53 43 49 49 20 74 65 78 74";
  REQUIRE_EQ(result_upper, expected_upper);

  // Test lowercase
  std::string const result_lower = quill::utility::to_hex(buffer.data(), buffer.length(), false);
  std::string const expected_lower = "41 20 6c 6f 6e 67 65 72 20 41 53 43 49 49 20 74 65 78 74";
  REQUIRE_EQ(result_lower, expected_lower);
}

/***/
TEST_CASE("byte_buffer_to_hex_uppercase_and_lowercase")
{
  uint32_t input = 431234;
  unsigned char buffer[4];
  std::memcpy(buffer, reinterpret_cast<char*>(&input), sizeof(input));

  // Test uppercase
  std::string const result_upper = quill::utility::to_hex(buffer, 4, true);
  std::string const expected_upper = "82 94 06 00";
  REQUIRE_EQ(result_upper, expected_upper);

  // Test lowercase
  std::string const result_lower = quill::utility::to_hex(buffer, 4, false);
  std::string const expected_lower = "82 94 06 00";
  REQUIRE_EQ(result_lower, expected_lower);

  // Test with const buffer
  unsigned char* const buffer_const = &buffer[0];

  // Uppercase
  std::string const result_const_upper = quill::utility::to_hex(buffer_const, 4, true);
  REQUIRE_EQ(result_const_upper, expected_upper);

  // Lowercase
  std::string const result_const_lower = quill::utility::to_hex(buffer_const, 4, false);
  REQUIRE_EQ(result_const_lower, expected_lower);
}

TEST_SUITE_END();