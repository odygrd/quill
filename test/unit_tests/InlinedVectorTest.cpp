#include "doctest/doctest.h"

#include "quill/core/Common.h"
#include "quill/core/InlinedVector.h"

#include <vector>

#if defined(__GNUC__) && !defined(__clang__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

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

TEST_CASE("assign_access_index")
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

  // Assign a new value to index 2
  vec.assign(2, 199);

  // Assign value to invalid index
  REQUIRE_THROWS(vec.assign(3, 199));

  // Verify that the value has been updated
  REQUIRE_EQ(vec[0], 10);
  REQUIRE_EQ(vec[1], 99);
  REQUIRE_EQ(vec[2], 199);

  {
    bool throws{false};

    try
    {
      // we only pushed_back 3 elements, trying to access the 4th will fail
      uint32_t const elem = vec[3];
      (void)elem;
    }
    catch (std::exception const&)
    {
      throws = true;
    }

    REQUIRE(throws);
  }

  // Add one more
  vec.push_back(130);

  // Verify that the values
  REQUIRE_EQ(vec[0], 10);
  REQUIRE_EQ(vec[1], 99);
  REQUIRE_EQ(vec[2], 199);
  REQUIRE_EQ(vec[3], 130);

  // Add one more - vector will switch to heap
  vec.push_back(230);

  // Verify that the values
  REQUIRE_EQ(vec[0], 10);
  REQUIRE_EQ(vec[1], 99);
  REQUIRE_EQ(vec[2], 199);
  REQUIRE_EQ(vec[3], 130);
  REQUIRE_EQ(vec[4], 230);

  {
    bool throws{false};

    try
    {
      // we only pushed_back 5 elements, trying to access the 6th will fail
      uint32_t const elem = vec[5];
      (void)elem;
    }
    catch (std::exception const&)
    {
      throws = true;
    }

    REQUIRE(throws);
  }
}

TEST_CASE("capacity_exactly_powers_of_two")
{
  // Test with N=2 to ensure doubling works correctly
  InlinedVector<uint32_t, 2> vec;

  vec.push_back(1);
  vec.push_back(2);
  REQUIRE_EQ(vec.size(), 2);
  REQUIRE_EQ(vec.capacity(), 2);
  REQUIRE_EQ(vec[0], 1);
  REQUIRE_EQ(vec[1], 2);

  // This should trigger reallocation to capacity 4
  vec.push_back(3);
  REQUIRE_EQ(vec.size(), 3);
  REQUIRE_EQ(vec.capacity(), 4);
  REQUIRE_EQ(vec[0], 1);
  REQUIRE_EQ(vec[1], 2);
  REQUIRE_EQ(vec[2], 3);

  vec.push_back(4);
  REQUIRE_EQ(vec.size(), 4);
  REQUIRE_EQ(vec.capacity(), 4);

  // This should trigger reallocation to capacity 8
  vec.push_back(5);
  REQUIRE_EQ(vec.size(), 5);
  REQUIRE_EQ(vec.capacity(), 8);
  REQUIRE_EQ(vec[0], 1);
  REQUIRE_EQ(vec[1], 2);
  REQUIRE_EQ(vec[2], 3);
  REQUIRE_EQ(vec[3], 4);
  REQUIRE_EQ(vec[4], 5);
}

TEST_CASE("assign_after_heap_allocation")
{
  InlinedVector<uint32_t, 2> vec;

  // Fill inline buffer
  vec.push_back(10);
  vec.push_back(20);

  // Trigger heap allocation
  vec.push_back(30);
  vec.push_back(40);

  REQUIRE_EQ(vec.capacity(), 4);

  // Now test assign on heap-allocated storage
  vec.assign(0, 100);
  vec.assign(1, 200);
  vec.assign(2, 300);
  vec.assign(3, 400);

  REQUIRE_EQ(vec[0], 100);
  REQUIRE_EQ(vec[1], 200);
  REQUIRE_EQ(vec[2], 300);
  REQUIRE_EQ(vec[3], 400);

  // Test assign at boundary
  vec.assign(3, 999);
  REQUIRE_EQ(vec[3], 999);

  // Test that assign beyond size still throws
  REQUIRE_THROWS(vec.assign(4, 500));
}

TEST_CASE("multiple_reallocations")
{
  InlinedVector<uint32_t, 1> vec;

  // This will cause multiple reallocations: 1 -> 2 -> 4 -> 8 -> 16
  for (uint32_t i = 0; i < 16; ++i)
  {
    vec.push_back(i * 10);
  }

  REQUIRE_EQ(vec.size(), 16);
  REQUIRE_EQ(vec.capacity(), 16);

  // Verify all values are correct after multiple reallocations
  for (uint32_t i = 0; i < 16; ++i)
  {
    REQUIRE_EQ(vec[i], i * 10);
  }

  // Add one more to trigger another reallocation
  vec.push_back(160);
  REQUIRE_EQ(vec.size(), 17);
  REQUIRE_EQ(vec.capacity(), 32);

  // Verify all values again
  for (uint32_t i = 0; i < 16; ++i)
  {
    REQUIRE_EQ(vec[i], i * 10);
  }
  REQUIRE_EQ(vec[16], 160);
}

TEST_CASE("clear_and_reuse_with_heap")
{
  InlinedVector<uint32_t, 2> vec;

  // Allocate heap
  for (uint32_t i = 0; i < 10; ++i)
  {
    vec.push_back(i);
  }

  size_t cap_before_clear = vec.capacity();
  vec.clear();

  REQUIRE_EQ(vec.size(), 0);
  REQUIRE_EQ(vec.capacity(), cap_before_clear);

  // Reuse the heap buffer
  for (uint32_t i = 100; i < 110; ++i)
  {
    vec.push_back(i);
  }

  REQUIRE_EQ(vec.size(), 10);
  for (uint32_t i = 0; i < 10; ++i)
  {
    REQUIRE_EQ(vec[i], 100 + i);
  }
}

TEST_CASE("zero_value_storage")
{
  InlinedVector<uint32_t, 4> vec;

  // Push zeros
  vec.push_back(0);
  vec.push_back(0);
  vec.push_back(0);
  vec.push_back(0);

  REQUIRE_EQ(vec.size(), 4);
  for (size_t i = 0; i < 4; ++i)
  {
    REQUIRE_EQ(vec[i], 0);
  }

  // Trigger heap allocation with zeros
  vec.push_back(0);
  REQUIRE_EQ(vec.size(), 5);
  REQUIRE_GT(vec.capacity(), 4);

  for (size_t i = 0; i < 5; ++i)
  {
    REQUIRE_EQ(vec[i], 0);
  }
}

TEST_CASE("boundary_access_inline_only")
{
  InlinedVector<uint32_t, 8> vec;

  // Fill to capacity but not beyond
  for (uint32_t i = 0; i < 8; ++i)
  {
    vec.push_back(i);
  }

  REQUIRE_EQ(vec.size(), 8);
  REQUIRE_EQ(vec.capacity(), 8);

  // Access all elements including boundaries
  REQUIRE_EQ(vec[0], 0);
  REQUIRE_EQ(vec[7], 7);

  // Access beyond size should throw
  REQUIRE_THROWS(vec[8]);

  // Assign at boundaries
  vec.assign(0, 100);
  vec.assign(7, 700);

  REQUIRE_EQ(vec[0], 100);
  REQUIRE_EQ(vec[7], 700);
}

TEST_CASE("alternating_push_assign")
{
  InlinedVector<uint32_t, 3> vec;

  vec.push_back(1);
  vec.assign(0, 10);
  REQUIRE_EQ(vec[0], 10);

  vec.push_back(2);
  vec.assign(1, 20);
  REQUIRE_EQ(vec[0], 10);
  REQUIRE_EQ(vec[1], 20);

  vec.push_back(3);
  vec.assign(2, 30);

  // Now at capacity, next push will allocate heap
  vec.push_back(4);
  REQUIRE_GT(vec.capacity(), 3);

  vec.assign(3, 40);

  REQUIRE_EQ(vec[0], 10);
  REQUIRE_EQ(vec[1], 20);
  REQUIRE_EQ(vec[2], 30);
  REQUIRE_EQ(vec[3], 40);

  vec.push_back(5);
  vec.assign(4, 50);

  REQUIRE_EQ(vec[0], 10);
  REQUIRE_EQ(vec[1], 20);
  REQUIRE_EQ(vec[2], 30);
  REQUIRE_EQ(vec[3], 40);
  REQUIRE_EQ(vec[4], 50);
}

TEST_CASE("test_heap_buffer_initialization")
{
  InlinedVector<uint32_t, 2> vec;

  vec.push_back(1);
  vec.push_back(2);
  vec.push_back(3);

  REQUIRE_THROWS(vec[3]);
}

TEST_CASE("test_return_value_of_push_back")
{
  InlinedVector<uint32_t, 2> vec;

  uint32_t ret1 = vec.push_back(42);
  REQUIRE_EQ(ret1, 42);

  uint32_t ret2 = vec.push_back(99);
  REQUIRE_EQ(ret2, 99);

  // Trigger heap allocation
  uint32_t ret3 = vec.push_back(123);
  REQUIRE_EQ(ret3, 123);
}

TEST_CASE("test_clear_multiple_times")
{
  InlinedVector<uint32_t, 2> vec;

  for (int round = 0; round < 5; ++round)
  {
    vec.clear();
    REQUIRE_EQ(vec.size(), 0);

    vec.push_back(100 + round);
    vec.push_back(200 + round);
    vec.push_back(300 + round);

    REQUIRE_EQ(vec[0], 100 + round);
    REQUIRE_EQ(vec[1], 200 + round);
    REQUIRE_EQ(vec[2], 300 + round);
  }
}

TEST_CASE("assign_at_index_zero_single_element")
{
  InlinedVector<uint32_t, 4> vec;

  vec.push_back(100);
  vec.assign(0, 999);
  REQUIRE_EQ(vec[0], 999);
  REQUIRE_EQ(vec.size(), 1);
}

TEST_CASE("exact_capacity_boundary")
{
  InlinedVector<uint32_t, 4> vec;

  // Fill exactly to inline capacity
  vec.push_back(1);
  vec.push_back(2);
  vec.push_back(3);
  vec.push_back(4);

  REQUIRE_EQ(vec.size(), 4);
  REQUIRE_EQ(vec.capacity(), 4);

  // Now push one more - should trigger heap
  vec.push_back(5);

  REQUIRE_EQ(vec.size(), 5);
  REQUIRE_EQ(vec.capacity(), 8);

  // All values should still be correct
  for (uint32_t i = 0; i < 5; ++i)
  {
    REQUIRE_EQ(vec[i], i + 1);
  }
}

TEST_CASE("reallocation_preserves_all_data")
{
  InlinedVector<uint32_t, 1> vec;

  std::vector<uint32_t> reference;

  // Stress test: many reallocations
  for (uint32_t i = 0; i < 100; ++i)
  {
    vec.push_back(i * 7 + 13); // Some arbitrary values
    reference.push_back(i * 7 + 13);
  }

  REQUIRE_EQ(vec.size(), reference.size());

  // Verify every single value
  for (size_t i = 0; i < reference.size(); ++i)
  {
    REQUIRE_EQ(vec[i], reference[i]);
  }
}

TEST_CASE("clear_doesnt_reset_capacity")
{
  InlinedVector<uint32_t, 2> vec;

  // Force heap allocation
  for (uint32_t i = 0; i < 10; ++i)
  {
    vec.push_back(i);
  }

  size_t cap_after_growth = vec.capacity();
  REQUIRE_GT(cap_after_growth, 2);

  vec.clear();

  // Capacity should remain the same
  REQUIRE_EQ(vec.capacity(), cap_after_growth);
  REQUIRE_EQ(vec.size(), 0);

  // After clear, we should be able to push again
  vec.push_back(999);
  REQUIRE_EQ(vec[0], 999);
  REQUIRE_EQ(vec.size(), 1);
}

TEST_CASE("constructor_initializes_inline_buffer")
{
  // Constructor initializes all inline buffer elements to value_type{}
  // For uint32_t, this should be 0
  InlinedVector<uint32_t, 5> vec;

  // Size is 0, so we can't access elements
  REQUIRE_EQ(vec.size(), 0);
  REQUIRE_THROWS(vec[0]);
}

TEST_CASE("large_inline_capacity")
{
  // Test with large inline capacity to ensure no issues
  InlinedVector<uint32_t, 1000> vec;

  // Fill partially
  for (uint32_t i = 0; i < 500; ++i)
  {
    vec.push_back(i);
  }

  REQUIRE_EQ(vec.size(), 500);
  REQUIRE_EQ(vec.capacity(), 1000);

  // Verify values
  for (uint32_t i = 0; i < 500; ++i)
  {
    REQUIRE_EQ(vec[i], i);
  }
}

TEST_CASE("copy_loop_correctness_inline_to_heap")
{
  InlinedVector<uint32_t, 3> vec;

  // Set specific values in inline storage
  vec.push_back(111);
  vec.push_back(222);
  vec.push_back(333);

  REQUIRE_EQ(vec.capacity(), 3);

  // This triggers the copy from inline to heap
  vec.push_back(444);

  REQUIRE_GT(vec.capacity(), 3);

  // The copy loop should have copied all 3 elements
  REQUIRE_EQ(vec[0], 111);
  REQUIRE_EQ(vec[1], 222);
  REQUIRE_EQ(vec[2], 333);
  REQUIRE_EQ(vec[3], 444);
}

TEST_CASE("copy_loop_correctness_heap_to_heap")
{
  InlinedVector<uint32_t, 1> vec;

  // Force heap allocation (capacity becomes 2)
  vec.push_back(10);
  vec.push_back(20);

  REQUIRE_EQ(vec.capacity(), 2);

  // Force another reallocation (capacity becomes 4)
  vec.push_back(30);

  REQUIRE_EQ(vec.capacity(), 4);
  REQUIRE_EQ(vec[0], 10);
  REQUIRE_EQ(vec[1], 20);
  REQUIRE_EQ(vec[2], 30);

  // Force yet another reallocation (capacity becomes 8)
  vec.push_back(40);
  vec.push_back(50);

  REQUIRE_EQ(vec.capacity(), 8);
  REQUIRE_EQ(vec[0], 10);
  REQUIRE_EQ(vec[1], 20);
  REQUIRE_EQ(vec[2], 30);
  REQUIRE_EQ(vec[3], 40);
  REQUIRE_EQ(vec[4], 50);
}

TEST_CASE("clear_after_inline_only")
{
  InlinedVector<uint32_t, 10> vec;

  vec.push_back(1);
  vec.push_back(2);
  vec.push_back(3);

  REQUIRE_EQ(vec.size(), 3);
  REQUIRE_EQ(vec.capacity(), 10);

  vec.clear();

  REQUIRE_EQ(vec.size(), 0);
  REQUIRE_EQ(vec.capacity(), 10);

  // After clear, when we push, are we writing to the right place?
  vec.push_back(100);
  vec.push_back(200);

  REQUIRE_EQ(vec[0], 100);
  REQUIRE_EQ(vec[1], 200);
}

TEST_CASE("value_at_size_minus_one")
{
  InlinedVector<uint32_t, 4> vec;

  vec.push_back(10);
  REQUIRE_EQ(vec[vec.size() - 1], 10);

  vec.push_back(20);
  REQUIRE_EQ(vec[vec.size() - 1], 20);

  vec.push_back(30);
  REQUIRE_EQ(vec[vec.size() - 1], 30);
}

TEST_CASE("first_reallocation_boundary")
{
  // This tests the exact moment we transition from inline to heap
  InlinedVector<uint32_t, 3> vec;

  // Fill to capacity
  vec.push_back(100);
  REQUIRE_EQ(vec.capacity(), 3);
  REQUIRE_EQ(vec.size(), 1);
  REQUIRE_EQ(vec[0], 100);

  vec.push_back(200);
  REQUIRE_EQ(vec.capacity(), 3);
  REQUIRE_EQ(vec.size(), 2);
  REQUIRE_EQ(vec[0], 100);
  REQUIRE_EQ(vec[1], 200);

  vec.push_back(300);
  REQUIRE_EQ(vec.capacity(), 3);
  REQUIRE_EQ(vec.size(), 3);
  REQUIRE_EQ(vec[0], 100);
  REQUIRE_EQ(vec[1], 200);
  REQUIRE_EQ(vec[2], 300);

  // This triggers inline -> heap transition
  vec.push_back(400);
  REQUIRE_EQ(vec.capacity(), 6);
  REQUIRE_EQ(vec.size(), 4);

  // ALL previous values must still be correct after the copy
  REQUIRE_EQ(vec[0], 100);
  REQUIRE_EQ(vec[1], 200);
  REQUIRE_EQ(vec[2], 300);
  REQUIRE_EQ(vec[3], 400);
}

TEST_CASE("assign_exactly_at_size_minus_one")
{
  InlinedVector<uint32_t, 3> vec;

  vec.push_back(1);
  vec.push_back(2);
  vec.push_back(3);

  // Assign at last element (size - 1)
  vec.assign(2, 999);
  REQUIRE_EQ(vec[2], 999);
  REQUIRE_EQ(vec[0], 1);
  REQUIRE_EQ(vec[1], 2);

  // Now trigger heap allocation
  vec.push_back(4);

  // Verify all values after heap transition
  REQUIRE_EQ(vec[0], 1);
  REQUIRE_EQ(vec[1], 2);
  REQUIRE_EQ(vec[2], 999); // This was assigned
  REQUIRE_EQ(vec[3], 4);
}

TEST_CASE("rapid_clear_and_refill")
{
  InlinedVector<uint32_t, 2> vec;

  for (int iteration = 0; iteration < 10; ++iteration)
  {
    vec.clear();

    // Fill beyond inline capacity
    vec.push_back(iteration * 100 + 1);
    vec.push_back(iteration * 100 + 2);
    vec.push_back(iteration * 100 + 3);
    vec.push_back(iteration * 100 + 4);

    REQUIRE_EQ(vec.size(), 4);
    REQUIRE_EQ(vec[0], iteration * 100 + 1);
    REQUIRE_EQ(vec[1], iteration * 100 + 2);
    REQUIRE_EQ(vec[2], iteration * 100 + 3);
    REQUIRE_EQ(vec[3], iteration * 100 + 4);
  }
}

TEST_CASE("push_at_exact_power_of_two_boundaries")
{
  InlinedVector<uint32_t, 4> vec;

  // Size 0, capacity 4
  REQUIRE_EQ(vec.size(), 0);
  REQUIRE_EQ(vec.capacity(), 4);

  // Fill to size 4
  for (uint32_t i = 0; i < 4; ++i)
  {
    vec.push_back(i);
  }
  REQUIRE_EQ(vec.size(), 4);
  REQUIRE_EQ(vec.capacity(), 4);

  // Push 5th element: triggers resize to 8
  vec.push_back(4);
  REQUIRE_EQ(vec.size(), 5);
  REQUIRE_EQ(vec.capacity(), 8);

  // Fill to size 8
  for (uint32_t i = 5; i < 8; ++i)
  {
    vec.push_back(i);
  }
  REQUIRE_EQ(vec.size(), 8);
  REQUIRE_EQ(vec.capacity(), 8);

  // Push 9th element: triggers resize to 16
  vec.push_back(8);
  REQUIRE_EQ(vec.size(), 9);
  REQUIRE_EQ(vec.capacity(), 16);

  // Verify all values
  for (uint32_t i = 0; i < 9; ++i)
  {
    REQUIRE_EQ(vec[i], i);
  }
}

TEST_CASE("interleaved_operations_stress")
{
  InlinedVector<uint32_t, 2> vec;

  vec.push_back(10);
  REQUIRE_EQ(vec[0], 10);

  vec.push_back(20);
  REQUIRE_EQ(vec[1], 20);

  vec.assign(0, 11);
  REQUIRE_EQ(vec[0], 11);

  vec.push_back(30); // Triggers heap
  REQUIRE_EQ(vec[0], 11);
  REQUIRE_EQ(vec[1], 20);
  REQUIRE_EQ(vec[2], 30);

  vec.assign(1, 22);
  REQUIRE_EQ(vec[0], 11);
  REQUIRE_EQ(vec[1], 22);
  REQUIRE_EQ(vec[2], 30);

  vec.push_back(40);
  REQUIRE_EQ(vec[3], 40);

  vec.push_back(50); // Triggers another resize
  REQUIRE_EQ(vec[0], 11);
  REQUIRE_EQ(vec[1], 22);
  REQUIRE_EQ(vec[2], 30);
  REQUIRE_EQ(vec[3], 40);
  REQUIRE_EQ(vec[4], 50);

  vec.assign(2, 33);
  REQUIRE_EQ(vec[2], 33);

  vec.clear();
  REQUIRE_EQ(vec.size(), 0);

  vec.push_back(100);
  REQUIRE_EQ(vec[0], 100);
  REQUIRE_EQ(vec.size(), 1);
}

TEST_CASE("size_one_vector_operations")
{
  InlinedVector<uint32_t, 1> vec;

  REQUIRE_EQ(vec.size(), 0);
  REQUIRE_EQ(vec.capacity(), 1);

  // Add first element - stays inline
  vec.push_back(42);
  REQUIRE_EQ(vec.size(), 1);
  REQUIRE_EQ(vec.capacity(), 1);
  REQUIRE_EQ(vec[0], 42);

  // Add second element - triggers heap (capacity -> 2)
  vec.push_back(43);
  REQUIRE_EQ(vec.size(), 2);
  REQUIRE_EQ(vec.capacity(), 2);
  REQUIRE_EQ(vec[0], 42);
  REQUIRE_EQ(vec[1], 43);

  // Add third element - triggers heap realloc (capacity -> 4)
  vec.push_back(44);
  REQUIRE_EQ(vec.size(), 3);
  REQUIRE_EQ(vec.capacity(), 4);
  REQUIRE_EQ(vec[0], 42);
  REQUIRE_EQ(vec[1], 43);
  REQUIRE_EQ(vec[2], 44);

  // Assign operations
  vec.assign(0, 100);
  vec.assign(1, 101);
  vec.assign(2, 102);

  REQUIRE_EQ(vec[0], 100);
  REQUIRE_EQ(vec[1], 101);
  REQUIRE_EQ(vec[2], 102);
}

TEST_CASE("verify_heap_pointer_not_corrupted")
{
  InlinedVector<uint32_t, 2> vec;

  // Transition to heap
  vec.push_back(1);
  vec.push_back(2);
  vec.push_back(3);

  REQUIRE_GT(vec.capacity(), 2);

  // Do many operations to ensure heap pointer stays valid
  for (uint32_t i = 0; i < 100; ++i)
  {
    vec.push_back(i + 10);
  }

  // Verify first elements are still correct
  REQUIRE_EQ(vec[0], 1);
  REQUIRE_EQ(vec[1], 2);
  REQUIRE_EQ(vec[2], 3);

  // Verify last elements
  REQUIRE_EQ(vec[vec.size() - 1], 109);
  REQUIRE_EQ(vec[vec.size() - 2], 108);
}

TEST_CASE("empty_vector_operations")
{
  InlinedVector<uint32_t, 5> vec;

  // Should not be able to access anything
  REQUIRE_THROWS(vec[0]);
  REQUIRE_THROWS(vec.assign(0, 100));

  REQUIRE_EQ(vec.size(), 0);
  REQUIRE_EQ(vec.capacity(), 5);

  // Clear on empty vector should be safe
  vec.clear();
  REQUIRE_EQ(vec.size(), 0);
}

TEST_CASE("assign_after_multiple_reallocations")
{
  InlinedVector<uint32_t, 1> vec;

  // Force multiple reallocations
  for (uint32_t i = 0; i < 20; ++i)
  {
    vec.push_back(i * 10);
  }

  // Now assign to various indices
  vec.assign(0, 1000);
  vec.assign(10, 2000);
  vec.assign(19, 3000);

  REQUIRE_EQ(vec[0], 1000);
  REQUIRE_EQ(vec[10], 2000);
  REQUIRE_EQ(vec[19], 3000);

  // Other values should be unchanged
  REQUIRE_EQ(vec[1], 10);
  REQUIRE_EQ(vec[5], 50);
  REQUIRE_EQ(vec[15], 150);
}

TEST_SUITE_END();

#if defined(__GNUC__) && !defined(__clang__)
  #pragma GCC diagnostic pop
#endif