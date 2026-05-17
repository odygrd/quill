#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/core/MathUtilities.h"

TEST_SUITE_BEGIN("MathUtilities");

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

TEST_SUITE_END();