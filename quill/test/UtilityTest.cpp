#include "quill/Utility.h"
#include <cstring>
#include <gtest/gtest.h>

/***/
TEST(Utility, ascii_string_to_hex_1)
{
  std::string buffer = "Hello World";
  std::string const result = quill::utility::to_hex(buffer.data(), buffer.length());
  std::string const expected = "48 65 6C 6C 6F 20 57 6F 72 6C 64";
  EXPECT_EQ(result, expected);
}

/***/
TEST(Utility, ascii_string_to_hex_2)
{
  std::string buffer = "A longer ASCII text";
  std::string const result = quill::utility::to_hex(buffer.data(), buffer.length());
  std::string const expected = "41 20 6C 6F 6E 67 65 72 20 41 53 43 49 49 20 74 65 78 74";
  EXPECT_EQ(result, expected);
}

/***/
TEST(Utility, ascii_string_to_hex_2_const)
{
  std::string const buffer = "A longer ASCII text";
  std::string const result = quill::utility::to_hex(buffer.data(), buffer.length());
  std::string const expected = "41 20 6C 6F 6E 67 65 72 20 41 53 43 49 49 20 74 65 78 74";
  EXPECT_EQ(result, expected);
}

/***/
TEST(Utility, byte_buffer_to_hex_1)
{
  uint32_t input = 431234;
  unsigned char buffer[4];
  std::memcpy(buffer, reinterpret_cast<char*>(&input), sizeof(input));

  // non const overload
  std::string const result = quill::utility::to_hex(buffer, 4);
  std::string const expected = "82 94 06 00";
  EXPECT_EQ(result, expected);

  // const overload
  unsigned char* const buffer_const = &buffer[0];
  std::string const result_2 = quill::utility::to_hex(buffer_const, 4);
  EXPECT_EQ(result_2, expected);
}