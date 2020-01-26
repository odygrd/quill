#include <gtest/gtest.h>

#include "quill/Utility.h"

/***/
TEST(Utility, ascii_string_to_hex_1)
{
  std::string buffer = "Hello World";
  std::string result = quill::utility::to_hex(buffer.data(), buffer.length());
  std::string expected = "48 65 6C 6C 6F 20 57 6F 72 6C 64";
  EXPECT_EQ(result, expected);
}

/***/
TEST(Utility, ascii_string_to_hex_2)
{
  std::string buffer = "A longer ASCII text";
  std::string result = quill::utility::to_hex(buffer.data(), buffer.length());
  std::string expected = "41 20 6C 6F 6E 67 65 72 20 41 53 43 49 49 20 74 65 78 74";
  EXPECT_EQ(result, expected);
}