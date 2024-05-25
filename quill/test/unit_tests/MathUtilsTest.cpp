#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/core/MathUtils.h"

TEST_SUITE_BEGIN("MathUtils");

using namespace quill::detail;
using namespace quill;

TEST_CASE("is_power_of_two")
{
  // Test with power of two numbers
  CHECK(is_power_of_two(1) == true);
  CHECK(is_power_of_two(2) == true);
  CHECK(is_power_of_two(4) == true);
  CHECK(is_power_of_two(8) == true);
  CHECK(is_power_of_two(16) == true);
  CHECK(is_power_of_two(1024) == true);

  // Test with non-power of two numbers
  CHECK(is_power_of_two(0) == false);
  CHECK(is_power_of_two(3) == false);
  CHECK(is_power_of_two(5) == false);
  CHECK(is_power_of_two(6) == false);
  CHECK(is_power_of_two(7) == false);
  CHECK(is_power_of_two(9) == false);

  // Test edge cases
  CHECK(is_power_of_two(std::numeric_limits<std::size_t>::max()) == false);
}

TEST_CASE("next_power_of_two_unsigned")
{
  // Test with numbers that are already powers of two
  CHECK(next_power_of_two(1u) == 1u);
  CHECK(next_power_of_two(2u) == 2u);
  CHECK(next_power_of_two(4u) == 4u);
  CHECK(next_power_of_two(8u) == 8u);
  CHECK(next_power_of_two(16u) == 16u);
  CHECK(next_power_of_two(1024u) == 1024u);

  // Test with numbers that are not powers of two
  CHECK(next_power_of_two(0u) == 1u);
  CHECK(next_power_of_two(3u) == 4u);
  CHECK(next_power_of_two(5u) == 8u);
  CHECK(next_power_of_two(6u) == 8u);
  CHECK(next_power_of_two(7u) == 8u);
  CHECK(next_power_of_two(9u) == 16u);

  // Test edge cases
  constexpr std::size_t max_power_of_2 = (std::numeric_limits<std::size_t>::max() >> 1) + 1;
  CHECK(next_power_of_two(std::numeric_limits<std::size_t>::max() - 1) == max_power_of_2); // Handling near overflow case
  CHECK(next_power_of_two(std::numeric_limits<std::size_t>::max() / 2 + 1) == max_power_of_2); // Largest possible input
}

TEST_CASE("next_power_of_two_signed")
{
  // Test with positive numbers that are already powers of two
  CHECK(next_power_of_two(1) == 1);
  CHECK(next_power_of_two(2) == 2);
  CHECK(next_power_of_two(4) == 4);
  CHECK(next_power_of_two(8) == 8);
  CHECK(next_power_of_two(16) == 16);
  CHECK(next_power_of_two(1024) == 1024);

  // Test with positive numbers that are not powers of two
  CHECK(next_power_of_two(0) == 1);
  CHECK(next_power_of_two(3) == 4);
  CHECK(next_power_of_two(5) == 8);
  CHECK(next_power_of_two(6) == 8);
  CHECK(next_power_of_two(7) == 8);
  CHECK(next_power_of_two(9) == 16);

  // Test edge cases
  constexpr int max_power_of_2_signed = (std::numeric_limits<int>::max() >> 1) + 1;
  CHECK(next_power_of_two(std::numeric_limits<int>::max() - 1) == max_power_of_2_signed); // Handling near overflow case for int
  CHECK(next_power_of_two(std::numeric_limits<int>::max() / 2 + 1) == max_power_of_2_signed); // Middle large input for int
}

TEST_SUITE_END();