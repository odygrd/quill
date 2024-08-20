#include "doctest/doctest.h"

#include "quill/core/Common.h"
#include "quill/core/InlinedVector.h"

TEST_SUITE_BEGIN("InlinedVector");

using namespace quill;
using namespace quill::detail;

TEST_CASE("basic_operations")
{
  InlinedVector<uint32_t, 4> vec; // Initial inline capacity of 4

  // Test initial size and capacity
  REQUIRE_EQ(vec.size(), 0);
  REQUIRE_EQ(vec.capacity(), 4);

  // Test push_back and size increase
  vec.push_back(10);
  vec.push_back(20);
  vec.push_back(30);
  vec.push_back(40);

  // Size and capacity after adding 4 elements
  REQUIRE_EQ(vec.size(), 4);
  REQUIRE_EQ(vec.capacity(), 4);

  // Push back more elements to trigger heap allocation
  vec.push_back(50);
  vec.push_back(60);

  // Size and capacity after adding 6 elements
  REQUIRE_EQ(vec.size(), 6);
  REQUIRE_GT(vec.capacity(), 4); // Should be greater than initial capacity

  // Test element access
  REQUIRE_EQ(vec[0], 10);
  REQUIRE_EQ(vec[1], 20);
  REQUIRE_EQ(vec[2], 30);
  REQUIRE_EQ(vec[3], 40);
  REQUIRE_EQ(vec[4], 50);
  REQUIRE_EQ(vec[5], 60);
}

TEST_CASE("edge_cases")
{
  InlinedVector<uint64_t, 1> vec;

  vec.push_back(0);
  vec.push_back(1);

  REQUIRE_EQ(vec.size(), 2);
  REQUIRE_GT(vec.capacity(), 1); // Should have allocated heap memory

  // Test access to elements
  REQUIRE_EQ(vec[0], 0);
  REQUIRE_EQ(vec[1], 1);

  // Test clearing the vector
  vec.clear();
  REQUIRE_EQ(vec.size(), 0);
  REQUIRE_GT(vec.capacity(), 0); // Capacity should still be non-zero (heap memory should be preserved)

  // Test pushing after clear
  vec.push_back(2);
  REQUIRE_EQ(vec.size(), 1);
  REQUIRE_EQ(vec[0], 2);

  // Test large capacity
  InlinedVector<int, 1000> large_vec;
  for (size_t i = 0; i < 1000; ++i)
  {
    large_vec.push_back(static_cast<int>(i));
  }
  REQUIRE_EQ(large_vec.size(), 1000);
  REQUIRE_EQ(large_vec[999], 999);
}

TEST_CASE("stress_test")
{
  SizeCacheVector vec;

  for (uint32_t j = 0; j < 256; ++j)
  {
    for (uint32_t i = 0; i < 10000; ++i)
    {
      vec.push_back(j * i);
    }

    REQUIRE_EQ(vec.size(), 10000);

    for (uint32_t i = 0; i < 10000; ++i)
    {
      REQUIRE_EQ(vec[i], j * i);
    }

    vec.clear();

    // Check vector after clear
    REQUIRE_EQ(vec.size(), 0);
    REQUIRE_GT(vec.capacity(), 10000); // Capacity should remain unchanged
  }
}

TEST_CASE("assign_valid_index")
{
  // Create an InlinedVector with uint32_t and a small capacity
  InlinedVector<uint32_t, 4> vec;

  // Add elements to the vector
  vec.push_back(10);
  vec.push_back(20);
  vec.push_back(30);

  // Assign a new value to index 1
  vec.assign(1, 99);

  // Verify that the value has been updated
  REQUIRE_EQ(vec[0], 10);
  REQUIRE_EQ(vec[1], 99);
  REQUIRE_EQ(vec[2], 30);

  // Assign a new value to the last element (index 2)
  vec.assign(2, 199);

  // Verify that the value has been updated
  REQUIRE_EQ(vec[0], 10);
  REQUIRE_EQ(vec[1], 99);
  REQUIRE_EQ(vec[2], 199);
}

TEST_SUITE_END();