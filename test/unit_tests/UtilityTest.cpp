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
TEST_CASE("ascii_string_to_hex_uppercase_without_space_delimiters")
{
  std::string buffer = "Hello World";
  std::string const result = quill::utility::to_hex(buffer.data(), buffer.length(), true, false);
  std::string const expected = "48656C6C6F20576F726C64";
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
TEST_CASE("ascii_string_to_hex_lowercase_without_space_delimiters")
{
  std::string buffer = "Hello World";
  std::string const result = quill::utility::to_hex(buffer.data(), buffer.length(), false, false);
  std::string const expected = "48656c6c6f20576f726c64";
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

/***/
TEST_CASE("empty_buffer_to_hex")
{
  // Test with empty buffer
  std::string buffer = "";
  std::string const result = quill::utility::to_hex(buffer.data(), buffer.length());
  std::string const expected = "";
  REQUIRE_EQ(result, expected);

  // Test with nullptr
  uint8_t* ptr{nullptr};
  std::string const result_null = quill::utility::to_hex(ptr, 0);
  REQUIRE_EQ(result_null, expected);
}

/***/
TEST_CASE("single_byte_to_hex")
{
  // Test with a single byte
  unsigned char buffer[1] = {0xAB};
  
  // Uppercase
  std::string const result_upper = quill::utility::to_hex(buffer, 1, true);
  std::string const expected_upper = "AB";
  REQUIRE_EQ(result_upper, expected_upper);
  
  // Lowercase
  std::string const result_lower = quill::utility::to_hex(buffer, 1, false);
  std::string const expected_lower = "ab";
  REQUIRE_EQ(result_lower, expected_lower);
}

/***/
TEST_CASE("binary_data_to_hex")
{
  // Test with binary data containing zeros and special values
  unsigned char buffer[] = {0x00, 0xFF, 0x7F, 0x80};
  
  // Uppercase
  std::string const result_upper = quill::utility::to_hex(buffer, sizeof(buffer), true);
  std::string const expected_upper = "00 FF 7F 80";
  REQUIRE_EQ(result_upper, expected_upper);
  
  // Lowercase
  std::string const result_lower = quill::utility::to_hex(buffer, sizeof(buffer), false);
  std::string const expected_lower = "00 ff 7f 80";
  REQUIRE_EQ(result_lower, expected_lower);
}

/***/
TEST_CASE("char_array_to_hex")
{
  // Test with C-style char array
  char buffer[] = {'A', 'B', 'C', '\0', '\n'};
  
  std::string const result = quill::utility::to_hex(buffer, sizeof(buffer), true);
  std::string const expected = "41 42 43 00 0A";
  REQUIRE_EQ(result, expected);
}

/***/
TEST_CASE("size_greater_than_buffer")
{
  // This tests how the function handles the case where size is 
  // explicitly passed and might be longer than the actual buffer
  // (Note: This test might cause undefined behavior, use with caution)
  
  std::string buffer = "ABC";
  // Only convert the first 2 bytes of the 3-byte string
  std::string const result = quill::utility::to_hex(buffer.data(), 2);
  std::string const expected = "41 42";
  REQUIRE_EQ(result, expected);
}

/***/
TEST_CASE("different_byte_types")
{
  // Test with different byte-sized types
  uint8_t uint8_buffer[] = {0x01, 0x02, 0x03};
  int8_t int8_buffer[] = {0x01, 0x02, 0x03};
  char char_buffer[] = {0x01, 0x02, 0x03};
  signed char schar_buffer[] = {0x01, 0x02, 0x03};
  unsigned char uchar_buffer[] = {0x01, 0x02, 0x03};
  
  std::string const expected = "01 02 03";
  
  // Test each type
  REQUIRE_EQ(quill::utility::to_hex(uint8_buffer, 3), expected);
  REQUIRE_EQ(quill::utility::to_hex(int8_buffer, 3), expected);
  REQUIRE_EQ(quill::utility::to_hex(char_buffer, 3), expected);
  REQUIRE_EQ(quill::utility::to_hex(schar_buffer, 3), expected);
  REQUIRE_EQ(quill::utility::to_hex(uchar_buffer, 3), expected);
}

/***/
TEST_CASE("non_ascii_string_to_hex")
{
  unsigned char utf8_buffer[] = {0xD0, 0x9F, 0xD1, 0x80, 0xD0, 0xB8, 0xD0, 0xB2, 0xD0, 0xB5, 0xD1, 0x82};
  
  std::string const result = quill::utility::to_hex(utf8_buffer, sizeof(utf8_buffer));
  std::string const expected = "D0 9F D1 80 D0 B8 D0 B2 D0 B5 D1 82";
  REQUIRE_EQ(result, expected);
}

/***/
TEST_CASE("to_hex_different_buffer_sizes")
{
  // Test with buffer sizes that test loop unrolling edge cases

  // 1 byte (no unrolling)
  {
    unsigned char buffer[1] = {0xAB};

    // With spaces (meaningless for 1 byte)
    std::string const result_space = quill::utility::to_hex(buffer, 1, true, true);
    std::string const expected_space = "AB";
    REQUIRE_EQ(result_space, expected_space);

    // Without spaces
    std::string const result_no_space = quill::utility::to_hex(buffer, 1, true, false);
    std::string const expected_no_space = "AB";
    REQUIRE_EQ(result_no_space, expected_no_space);
  }

  // 3 bytes (smaller than unroll_count)
  {
    unsigned char buffer[3] = {0xAB, 0xCD, 0xEF};

    // With spaces
    std::string const result_space = quill::utility::to_hex(buffer, 3, true, true);
    std::string const expected_space = "AB CD EF";
    REQUIRE_EQ(result_space, expected_space);

    // Without spaces
    std::string const result_no_space = quill::utility::to_hex(buffer, 3, true, false);
    std::string const expected_no_space = "ABCDEF";
    REQUIRE_EQ(result_no_space, expected_no_space);
  }

  // 4 bytes (exactly unroll_count)
  {
    unsigned char buffer[4] = {0x01, 0x23, 0x45, 0x67};

    // With spaces
    std::string const result_space = quill::utility::to_hex(buffer, 4, true, true);
    std::string const expected_space = "01 23 45 67";
    REQUIRE_EQ(result_space, expected_space);

    // Without spaces
    std::string const result_no_space = quill::utility::to_hex(buffer, 4, true, false);
    std::string const expected_no_space = "01234567";
    REQUIRE_EQ(result_no_space, expected_no_space);
  }

  // 5 bytes (unroll + 1 remainder)
  {
    unsigned char buffer[5] = {0x01, 0x23, 0x45, 0x67, 0x89};

    // With spaces
    std::string const result_space = quill::utility::to_hex(buffer, 5, true, true);
    std::string const expected_space = "01 23 45 67 89";
    REQUIRE_EQ(result_space, expected_space);

    // Without spaces
    std::string const result_no_space = quill::utility::to_hex(buffer, 5, true, false);
    std::string const expected_no_space = "0123456789";
    REQUIRE_EQ(result_no_space, expected_no_space);
  }

  // 8 bytes (2 * unroll_count)
  {
    unsigned char buffer[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

    // With spaces
    std::string const result_space = quill::utility::to_hex(buffer, 8, true, true);
    std::string const expected_space = "01 23 45 67 89 AB CD EF";
    REQUIRE_EQ(result_space, expected_space);

    // Without spaces
    std::string const result_no_space = quill::utility::to_hex(buffer, 8, true, false);
    std::string const expected_no_space = "0123456789ABCDEF";
    REQUIRE_EQ(result_no_space, expected_no_space);
  }
}

/***/
TEST_CASE("to_hex_special_values")
{
  // Test with special byte values to ensure lookup table works correctly
  unsigned char buffer[] = {
    0x00,       // First value in table
    0x0F,       // End of first row
    0x10,       // Start of second row
    0x7F,       // End of ASCII range
    0x80,       // Start of extended ASCII
    0xFF        // Last value in table
  };

  // With spaces (uppercase)
  std::string const result_upper_space = quill::utility::to_hex(buffer, sizeof(buffer), true, true);
  std::string const expected_upper_space = "00 0F 10 7F 80 FF";
  REQUIRE_EQ(result_upper_space, expected_upper_space);

  // Without spaces (uppercase)
  std::string const result_upper_no_space = quill::utility::to_hex(buffer, sizeof(buffer), true, false);
  std::string const expected_upper_no_space = "000F107F80FF";
  REQUIRE_EQ(result_upper_no_space, expected_upper_no_space);

  // With spaces (lowercase)
  std::string const result_lower_space = quill::utility::to_hex(buffer, sizeof(buffer), false, true);
  std::string const expected_lower_space = "00 0f 10 7f 80 ff";
  REQUIRE_EQ(result_lower_space, expected_lower_space);

  // Without spaces (lowercase)
  std::string const result_lower_no_space = quill::utility::to_hex(buffer, sizeof(buffer), false, false);
  std::string const expected_lower_no_space = "000f107f80ff";
  REQUIRE_EQ(result_lower_no_space, expected_lower_no_space);
}

/***/
TEST_CASE("to_hex_large_buffer_with_unrolling")
{
  // Test with a large buffer to ensure loop unrolling works correctly
  constexpr size_t buffer_size = 128;
  unsigned char buffer[buffer_size];

  // Fill with a pattern
  for (size_t i = 0; i < buffer_size; ++i)
  {
    buffer[i] = static_cast<unsigned char>(i & 0xFF);
  }

  // With spaces
  std::string const result_space = quill::utility::to_hex(buffer, buffer_size, true, true);

  // Check length: for space_delim=true, length should be size*3-1
  REQUIRE_EQ(result_space.length(), buffer_size * 3 - 1);

  // Verify first few bytes
  REQUIRE(result_space.substr(0, 8) == "00 01 02");

  // Verify first 4 bytes (first unroll block)
  std::string first_block = result_space.substr(0, 11);  // 00 01 02 03
  REQUIRE(first_block == "00 01 02 03");

  // Verify second 4 bytes (second unroll block)
  std::string second_block = result_space.substr(12, 11);  // 04 05 06 07
  REQUIRE(second_block == "04 05 06 07");

  // Verify third 4 bytes (third unroll block)
  std::string third_block = result_space.substr(24, 11);  // 08 09 0A 0B
  REQUIRE(third_block == "08 09 0A 0B");

  // Verify last few bytes - the buffer has 0-127 (0x00-0x7F), so check the last three bytes
  std::string last_block = result_space.substr(result_space.length() - 8);
  REQUIRE(last_block == "7D 7E 7F");

  // Without spaces
  std::string const result_no_space = quill::utility::to_hex(buffer, buffer_size, true, false);

  // Check length: for space_delim=false, length should be size*2
  REQUIRE_EQ(result_no_space.length(), buffer_size * 2);

  // Verify first few bytes
  REQUIRE(result_no_space.substr(0, 6) == "000102");

  // Verify first 4 bytes (first unroll block)
  std::string first_block_ns = result_no_space.substr(0, 8);  // 00010203
  REQUIRE(first_block_ns == "00010203");

  // Verify second 4 bytes (second unroll block)
  std::string second_block_ns = result_no_space.substr(8, 8);  // 04050607
  REQUIRE(second_block_ns == "04050607");

  // Verify third 4 bytes (third unroll block)
  std::string third_block_ns = result_no_space.substr(16, 8);  // 08090A0B
  REQUIRE(third_block_ns == "08090A0B");

  // Verify last few bytes (the last 3 bytes: 0x7D, 0x7E, 0x7F)
  std::string last_block_ns = result_no_space.substr(result_no_space.length() - 6);
  REQUIRE(last_block_ns == "7D7E7F");
}

TEST_SUITE_END();