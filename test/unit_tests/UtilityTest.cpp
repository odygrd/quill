#include "doctest/doctest.h"

#include "quill/Utility.h"
#include <cstdint>
#include <cstring>

TEST_SUITE_BEGIN("Utility");

/***/
TEST_CASE("ascii_string_to_hex_1")
{
  std::string buffer = "Hello World";
  std::string const result = quill::utility::to_hex(buffer.data(), buffer.length());
  std::string const expected = "48 65 6C 6C 6F 20 57 6F 72 6C 64";
  REQUIRE_EQ(result, expected);
}

/***/
TEST_CASE("ascii_string_to_hex_2")
{
  std::string buffer = "A longer ASCII text";
  std::string const result = quill::utility::to_hex(buffer.data(), buffer.length());
  std::string const expected = "41 20 6C 6F 6E 67 65 72 20 41 53 43 49 49 20 74 65 78 74";
  REQUIRE_EQ(result, expected);
}

/***/
TEST_CASE("ascii_string_to_hex_2_const")
{
  std::string const buffer = "A longer ASCII text";
  std::string const result = quill::utility::to_hex(buffer.data(), buffer.length());
  std::string const expected = "41 20 6C 6F 6E 67 65 72 20 41 53 43 49 49 20 74 65 78 74";
  REQUIRE_EQ(result, expected);
}

/***/
TEST_CASE("byte_buffer_to_hex_1")
{
  uint32_t input = 431234;
  unsigned char buffer[4];
  std::memcpy(buffer, reinterpret_cast<char*>(&input), sizeof(input));

  // non const overload
  std::string const result = quill::utility::to_hex(buffer, 4);
  std::string const expected = "82 94 06 00";
  REQUIRE_EQ(result, expected);

  // const overload
  unsigned char* const buffer_const = &buffer[0];
  std::string const result_2 = quill::utility::to_hex(buffer_const, 4);
  REQUIRE_EQ(result_2, expected);
}

TEST_SUITE_END();
