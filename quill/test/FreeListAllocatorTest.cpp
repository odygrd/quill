#include "doctest/doctest.h"

#include "misc/DocTestExtensions.h"
#include "quill/detail/backend/FreeListAllocator.h"

#include <algorithm>
#include <iostream>
#include <numeric>
#include <random>
#include <type_traits>

TEST_SUITE_BEGIN("FreeListAllocator");

using namespace quill;
using namespace quill::detail;

/** A mock class to access private variables */
class FreeListAllocatorMock : public FreeListAllocator
{
public:
  using FreeListAllocator::FreeListAllocator;

  /**
   * Print blocks, for testing only
   * @return
   * first: a vector of all blocks in order with a pair [size,used]
   * second: total number of allocated blocks
   */
  std::pair<std::vector<std::pair<std::size_t, std::size_t>>, size_t> get_all_blocks() const
  {
    std::vector<std::pair<std::size_t, std::size_t>> results;

    Block* block = _requested_from_os.empty() ? nullptr : static_cast<Block*>(_requested_from_os.front());

    while (block != nullptr)
    {
      results.emplace_back(block->header.size, block->header.used);
      block = block->header.next;
    }

    return std::make_pair(results, _requested_from_os.size());
  }

  std::pair<std::vector<std::pair<std::size_t, std::size_t>>, size_t> get_all_blocks_reverse() const
  {
    std::vector<std::pair<std::size_t, std::size_t>> results;

    Block* block = _requested_from_os.empty() ? nullptr : static_cast<Block*>(_requested_from_os.front());

    while (block != nullptr)
    {
      Block* next_block = block->header.next;
      if (!next_block)
      {
        // block will be the last block
        break;
      }
      block = block->header.next;
    }

    // start from last block and go backwards
    while (block != nullptr)
    {
      results.emplace_back(block->header.size, block->header.used);
      block = block->header.previous;
    }

    return std::make_pair(results, _requested_from_os.size());
  }

  /**
   * Print freelist blocks, for testing only
   * @return
   * first: a vector of all freelist blocks in order with a pair [size,used]
   */
  std::vector<std::pair<std::size_t, std::size_t>> get_freelist() const
  {
    std::vector<std::pair<std::size_t, std::size_t>> results;

    for (auto const& size_block : _free_list)
    {
      std::vector<Block*> const& block_col = *(size_block.second);

      for (auto* b : block_col)
      {
        // Just a paranoid check that the key is equal to the header size
        REQUIRE_EQ(size_block.first, b->header.size);

        results.emplace_back(b->header.size, b->header.used);
      }
    }

    return results;
  }

  /**
   * Gets the size of the header from the free list allocator
   * used here because BlockHeader is protected
   */
  static size_t size_of_header() noexcept { return sizeof(BlockHeader); }

  /**
   * Adds padding to size (similar to what FreeListAllocator is using
   */
  static size_t add_padding(size_t s) noexcept
  {
    return (s + sizeof(std::max_align_t) - 1) & ~(sizeof(std::max_align_t) - 1);
  }

  //  void get_all_blocks1() const
  //  {
  //    Block* block = _requested_from_os.empty() ? nullptr : static_cast<Block*>(_requested_from_os.front());
  //
  //    while (block != nullptr)
  //    {
  //      std::cout << "[" << (void*)(block->header.previous) << ",{" << (void*)(block) << ","
  //                << (void*)(block->data) << ","
  //                << block->header.size << "," << block->header.used << "},"
  //                << (void*)(block->header.next)  << "]" << std::endl;
  //      block = block->header.next;
  //    }
  //    std::cout << std::endl;
  //  }
};

struct TestStruct
{
public:
  TestStruct() = default;
  TestStruct(uint32_t a, uint32_t b) : a(a), b(b){};

  ~TestStruct()
  {
    a = 0;
    b = 0;
  }

  uint32_t a{0};
  uint32_t b{0};
};

/***/
TEST_CASE("freelist_allocator_with_unique_ptr")
{
  constexpr size_t TOTAL_CAPACITY{512};
  FreeListAllocatorMock fla{TOTAL_CAPACITY};

  void* buffer = fla.allocate(sizeof(TestStruct));

  using custom_deleter_t = FreeListAllocatorDeleter<TestStruct>;
  using unique_ptr_t = std::unique_ptr<TestStruct, custom_deleter_t>;

  {
    unique_ptr_t ptr{new (buffer) TestStruct(1337, 7331), custom_deleter_t{&fla}};

    REQUIRE_EQ(ptr->a, 1337);
    REQUIRE_EQ(ptr->b, 7331);

    {
      // Check freelist blocks
      auto const freelist_res = fla.get_freelist();
      REQUIRE_EQ(freelist_res.size(), 1);

      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl,
                 fla.add_padding(TOTAL_CAPACITY) - fla.add_padding(sizeof(TestStruct)) - fla.size_of_header());
      REQUIRE_EQ(block_used_fl, 0);

      auto const results = fla.get_all_blocks();
      size_t const total_allocated_blocks = results.second;
      auto const& blocks = results.first;

      // we only expect 1 allocation
      REQUIRE_EQ(total_allocated_blocks, 1);

      // Check block
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, fla.add_padding(sizeof(TestStruct)));
      REQUIRE_EQ(block_used, 1);

      size_t const block_size2 = blocks[1].first;
      size_t const block_used2 = blocks[1].second;
      REQUIRE_EQ(block_size2,
                 fla.add_padding(TOTAL_CAPACITY) - fla.add_padding(sizeof(TestStruct)) - fla.size_of_header());
      REQUIRE_EQ(block_used2, 0);
    }
  }

  // unique_ptr gets deleted - out of scope
  {
    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    // Check freelist blocks
    size_t const block_size_fl = freelist_res[0].first;
    size_t const block_used_fl = freelist_res[0].second;
    REQUIRE_EQ(block_size_fl, 512);
    REQUIRE_EQ(block_used_fl, 0);

    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // we only expect 1 allocation
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check block
    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 512);
    REQUIRE_EQ(block_used, 0);
  }
}

/***/
TEST_CASE("allocate_deallocate_from_free_list_random_initial_capacity_ctor")
{
  // we will allocate some big memory and then perform random size allocations and deallocations
  std::random_device rd;
  std::mt19937 gen(rd());
  // we pick the numbers like this so we do not exceed the TOTAL_CAPACITY
  // NOTE: this test runs and allocates fix mem_size blocks of 48

  std::uniform_int_distribution<> iterations_dis(11, 20);
  constexpr size_t NUM_RUNS{20};

  // allocate and deallocate a big chunk of memory to reserve space
  constexpr size_t TOTAL_CAPACITY{16384};
  FreeListAllocatorMock fla{TOTAL_CAPACITY};

  {
    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    // Check freelist blocks
    size_t const block_size_fl = freelist_res[0].first;
    size_t const block_used_fl = freelist_res[0].second;
    REQUIRE_EQ(block_size_fl, fla.add_padding(TOTAL_CAPACITY));
    REQUIRE_EQ(block_used_fl, 0);

    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // we only expect 1 allocation
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check free block
    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, fla.add_padding(TOTAL_CAPACITY));
    REQUIRE_EQ(block_used, 0);
  }

  // We will randomly allocate and deallocate many times and then check that every time
  // we have the original full block of memory back
  std::vector<void*> allocated_ptrs;
  std::vector<size_t> allocated_sizes;

  for (size_t i = 0; i < NUM_RUNS; ++i)
  {
    size_t iterations = iterations_dis(gen);

    // allocate
    for (size_t j = 0; j < iterations; ++j)
    {
      size_t const mem_size = 48;

      // we store all allocated sizes to check later
      allocated_sizes.push_back(fla.add_padding(mem_size));

      void* p = fla.allocate(mem_size);
      allocated_ptrs.push_back(p);
    }

    {
      // we will accumulate all sizes and to each size we will add the size of the block header
      size_t const total_allocated_memory =
        std::accumulate(allocated_sizes.begin(), allocated_sizes.end(), static_cast<size_t>(0),
                        [&fla](size_t a, size_t b) { return a + b + fla.size_of_header(); });

      // Check freelist blocks
      auto const freelist_res = fla.get_freelist();
      REQUIRE_EQ(freelist_res.size(), 1);

      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, TOTAL_CAPACITY - total_allocated_memory);
      REQUIRE_EQ(block_used_fl, 0);

      auto const results = fla.get_all_blocks();
      size_t const total_allocated_blocks = results.second;
      auto const& blocks = results.first;

      // we only expect 1 allocation
      REQUIRE_EQ(total_allocated_blocks, 1);

      // Check all allocated blocks, we expect it all to be merged back to 1
      size_t b_size = blocks.size();
      REQUIRE_EQ(b_size, allocated_sizes.size() + 1); // + 1 for the free block!

      // Check all used blocks
      for (size_t j = 0; j < allocated_sizes.size(); ++j)
      {
        size_t const block_size = blocks[j].first;
        size_t const block_used = blocks[j].second;
        REQUIRE_EQ(block_size, allocated_sizes[j]);
        REQUIRE_EQ(block_used, 1);
      }

      // Check free block
      size_t const block_size = blocks[allocated_sizes.size()].first;
      size_t const block_used = blocks[allocated_sizes.size()].second;
      REQUIRE_EQ(block_size, TOTAL_CAPACITY - total_allocated_memory);
      REQUIRE_EQ(block_used, 0);
    }

    // deallocate all in random order
    size_t iterations2 = iterations_dis(gen);
    if (iterations2 >= iterations)
    {
      iterations2 = iterations - 1;
    }

    std::shuffle(allocated_ptrs.begin(), allocated_ptrs.end(), gen);
    for (size_t k = 0; k < 10; ++k)
    {
      void* p = allocated_ptrs[allocated_ptrs.size() - 1 - k];
      fla.deallocate(p);
    }

    for (size_t k = 0; k < 10; ++k)
    {
      allocated_ptrs.pop_back();
      // all pointers are the same size so we don't care about what we pop back
      allocated_sizes.pop_back();
    }

    {
      // we will accumulate all sizes and to each size we will add the size of the block header
      size_t const total_allocated_memory =
        std::accumulate(allocated_sizes.begin(), allocated_sizes.end(), static_cast<size_t>(0),
                        [&fla](size_t a, size_t b) { return a + b; });

      size_t const total_allocated_memory_with_header_size =
        std::accumulate(allocated_sizes.begin(), allocated_sizes.end(), static_cast<size_t>(0),
                        [&fla](size_t a, size_t b) { return a + b + fla.size_of_header(); });

      auto const results = fla.get_all_blocks();
      size_t const total_allocated_blocks = results.second;
      auto const& blocks = results.first;

      // we only expect 1 allocation
      REQUIRE_EQ(total_allocated_blocks, 1);

      // Check all allocated blocks, we expect it all to be merged back to 1
      size_t b_size = blocks.size();

      // Check all used blocks
      uint64_t used_size = 0;
      uint64_t free_size = 0;

      for (size_t j = 0; j < blocks.size(); ++j)
      {
        size_t const block_size = blocks[j].first;
        size_t const block_used = blocks[j].second;

        if (block_used)
        {
          used_size += block_size;
        }
        else
        {
          free_size += block_size;
        }
      }

      REQUIRE_EQ(used_size, total_allocated_memory);
      REQUIRE_EQ(free_size,
                 TOTAL_CAPACITY - total_allocated_memory - fla.size_of_header() * (blocks.size() - 1));
    }
  }

  // clean what is left
  for (auto* p : allocated_ptrs)
  {
    fla.deallocate(p);
  }

  {
    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    // Check freelist blocks
    size_t const block_size_fl = freelist_res[0].first;
    size_t const block_used_fl = freelist_res[0].second;
    REQUIRE_EQ(block_size_fl, TOTAL_CAPACITY);
    REQUIRE_EQ(block_used_fl, 0);

    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // we only expect 1 allocation
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks, we expect it all to be merged back to 1
    size_t b_size = blocks.size();
    REQUIRE_EQ(b_size, 1);

    // Check free block
    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, TOTAL_CAPACITY);
    REQUIRE_EQ(block_used, 0);
  }
}

/***/
TEST_CASE("allocate_deallocate_from_free_list_random_default_ctor")
{
  // we will allocate some big memory and then perform random size allocations and deallocations
  std::random_device rd;
  std::mt19937 gen(rd());
  // we pick the numbers like this so we do not exceed the TOTAL_CAPACITY
  std::uniform_int_distribution<> memory_size_dis(1, 256);
  std::uniform_int_distribution<> iterations_dis(1, 50);

  // allocate and deallocate a big chunk of memory to reserve space
  constexpr size_t TOTAL_CAPACITY{16384};
  FreeListAllocatorMock fla;
  void* tmp = fla.allocate(TOTAL_CAPACITY);
  fla.deallocate(tmp);

  constexpr size_t NUM_RUNS{50};

  // We will randomly allocate and deallocate many times and then check that every time
  // we have the original full block of memory back
  for (size_t i = 0; i < NUM_RUNS; ++i)
  {
    std::vector<void*> allocated_ptrs;
    size_t iterations = iterations_dis(gen);

    // allocate
    std::vector<size_t> allocated_sizes;
    for (size_t j = 0; j < iterations; ++j)
    {
      size_t const mem_size = memory_size_dis(gen);

      // we store all allocated sizes to check later
      allocated_sizes.push_back(fla.add_padding(mem_size));

      void* p = fla.allocate(mem_size);
      allocated_ptrs.push_back(p);
    }

    {
      // we will accumulate all sizes and to each size we will add the size of the block header
      size_t const total_allocated_memory =
        std::accumulate(allocated_sizes.begin(), allocated_sizes.end(), static_cast<size_t>(0),
                        [&fla](size_t a, size_t b) { return a + b + fla.size_of_header(); });

      // Check freelist blocks
      auto const freelist_res = fla.get_freelist();
      REQUIRE_EQ(freelist_res.size(), 1);

      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, TOTAL_CAPACITY - total_allocated_memory);
      REQUIRE_EQ(block_used_fl, 0);

      auto const results = fla.get_all_blocks();
      size_t const total_allocated_blocks = results.second;
      auto const& blocks = results.first;

      // we only expect 1 allocation
      REQUIRE_EQ(total_allocated_blocks, 1);

      // Check all allocated blocks, we expect it all to be merged back to 1
      size_t b_size = blocks.size();
      REQUIRE_EQ(b_size, allocated_sizes.size() + 1); // + 1 for the free block!

      // Check all used blocks
      for (size_t j = 0; j < allocated_sizes.size(); ++j)
      {
        size_t const block_size = blocks[j].first;
        size_t const block_used = blocks[j].second;
        REQUIRE_EQ(block_size, allocated_sizes[j]);
        REQUIRE_EQ(block_used, 1);
      }

      // Check free block
      size_t const block_size = blocks[allocated_sizes.size()].first;
      size_t const block_used = blocks[allocated_sizes.size()].second;
      REQUIRE_EQ(block_size, TOTAL_CAPACITY - total_allocated_memory);
      REQUIRE_EQ(block_used, 0);
    }

    // deallocate all in random order
    std::shuffle(allocated_ptrs.begin(), allocated_ptrs.end(), gen);
    for (auto* p : allocated_ptrs)
    {
      fla.deallocate(p);
    }

    // Check allocator internal state
    {
      // Check freelist blocks
      auto const freelist_res = fla.get_freelist();
      REQUIRE_EQ(freelist_res.size(), 1);

      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, TOTAL_CAPACITY);
      REQUIRE_EQ(block_used_fl, 0);

      auto const results = fla.get_all_blocks();
      size_t const total_allocated_blocks = results.second;
      auto const& blocks = results.first;

      // we only expect 1 allocation
      REQUIRE_EQ(total_allocated_blocks, 1);

      // Check all allocated blocks, we expect it all to be merged back to 1
      size_t b_size = blocks.size();
      REQUIRE_EQ(b_size, 1);

      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, TOTAL_CAPACITY);
      REQUIRE_EQ(block_used, 0);
    }
  }
}

/***/
TEST_CASE("allocate_deallocate_many_from_os")
{
  constexpr size_t MEMORY_S{128};
  constexpr size_t ITERATIONS{50};

  FreeListAllocatorMock fla;

  std::vector<void*> allocated_ptrs;

  for (size_t i = 0; i < ITERATIONS; ++i)
  {
    // request num blocks of memory of size memory_s
    void* p = fla.allocate(MEMORY_S);
    allocated_ptrs.push_back(p);
  }

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    REQUIRE_EQ(total_allocated_blocks, ITERATIONS);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), ITERATIONS);

    for (size_t j = 0; j < ITERATIONS; ++j)
    {
      size_t const block_size = blocks[j].first;
      size_t const block_used = blocks[j].second;
      REQUIRE_EQ(block_size, MEMORY_S);
      REQUIRE_EQ(block_used, 1);
    }
  }

  // Deallocate all pointers we allocated
  for (auto* p : allocated_ptrs)
  {
    fla.deallocate(p);
  }

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    REQUIRE_EQ(total_allocated_blocks, ITERATIONS);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), ITERATIONS);

    for (size_t j = 0; j < ITERATIONS; ++j)
    {
      size_t const block_size = blocks[j].first;
      size_t const block_used = blocks[j].second;
      REQUIRE_EQ(block_size, MEMORY_S);
      REQUIRE_EQ(block_used, 0);
    }
  }

  allocated_ptrs.clear();

  // Now we request 2 * MEMORY_S.
  // The old blocks are not enough so new ones will be allocated
  for (size_t i = 0; i < ITERATIONS; ++i)
  {
    // request num blocks of memory of size memory_s
    void* p = fla.allocate(2 * MEMORY_S);
    allocated_ptrs.push_back(p);
  }

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    REQUIRE_EQ(total_allocated_blocks, 2 * ITERATIONS);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 2 * ITERATIONS);

    // First blocks are unused
    for (size_t j = 0; j < ITERATIONS; ++j)
    {
      size_t const block_size = blocks[j].first;
      size_t const block_used = blocks[j].second;
      REQUIRE_EQ(block_size, MEMORY_S);
      REQUIRE_EQ(block_used, 0);
    }

    // Second blocks are allocated
    for (size_t j = ITERATIONS; j < 2 * ITERATIONS; ++j)
    {
      size_t const block_size = blocks[j].first;
      size_t const block_used = blocks[j].second;
      REQUIRE_EQ(block_size, 2 * MEMORY_S);
      REQUIRE_EQ(block_used, 1);
    }
  }

  // Deallocate all pointers we allocated
  for (auto* p : allocated_ptrs)
  {
    fla.deallocate(p);
  }

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    REQUIRE_EQ(total_allocated_blocks, 2 * ITERATIONS);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 2 * ITERATIONS);

    // First blocks are unused
    for (size_t j = 0; j < ITERATIONS; ++j)
    {
      size_t const block_size = blocks[j].first;
      size_t const block_used = blocks[j].second;
      REQUIRE_EQ(block_size, MEMORY_S);
      REQUIRE_EQ(block_used, 0);
    }

    // Second blocks are free
    for (size_t j = ITERATIONS; j < 2 * ITERATIONS; ++j)
    {
      size_t const block_size = blocks[j].first;
      size_t const block_used = blocks[j].second;
      REQUIRE_EQ(block_size, 2 * MEMORY_S);
      REQUIRE_EQ(block_used, 0);
    }
  }

  allocated_ptrs.clear();

  // We will allocate again but this time MEMORY_S / 2
  // This can fit and split the MEMORY_S blocks we had allocated initially
  for (size_t i = 0; i < ITERATIONS; ++i)
  {
    // request num blocks of memory of size memory_s
    void* p = fla.allocate(MEMORY_S / 2);
    allocated_ptrs.push_back(p);
  }

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    REQUIRE_EQ(total_allocated_blocks, 2 * ITERATIONS);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3 * ITERATIONS);

    // The blocks are used here, and they are split
    for (size_t j = 0; j < ITERATIONS; j += 2)
    {
      size_t const block_size1 = blocks[j].first;
      size_t const block_used1 = blocks[j].second;
      REQUIRE_EQ(block_size1, 64);
      REQUIRE_EQ(block_used1, 1);

      // The remaining block that was split
      // Here we split to 64 (above) and another 64 but 32 was used for the header size
      size_t const block_size2 = blocks[j + 1].first;
      size_t const block_used2 = blocks[j + 1].second;
      REQUIRE_EQ(block_size2, 32);
      REQUIRE_EQ(block_used2, 0);
    }

    // The bigger blocks are not allocated
    for (size_t j = 2 * ITERATIONS; j < 3 * ITERATIONS; ++j)
    {
      size_t const block_size = blocks[j].first;
      size_t const block_used = blocks[j].second;
      REQUIRE_EQ(block_size, 2 * MEMORY_S);
      REQUIRE_EQ(block_used, 0);
    }
  }

  // Deallocate all pointers we allocated
  for (auto* p : allocated_ptrs)
  {
    fla.deallocate(p);
  }

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    REQUIRE_EQ(total_allocated_blocks, 2 * ITERATIONS);

    // Check all allocated blocks, here the blocks are merged back again
    REQUIRE_EQ(blocks.size(), 2 * ITERATIONS);

    // First blocks are unused
    for (size_t j = 0; j < ITERATIONS; ++j)
    {
      size_t const block_size = blocks[j].first;
      size_t const block_used = blocks[j].second;
      REQUIRE_EQ(block_size, MEMORY_S);
      REQUIRE_EQ(block_used, 0);
    }

    // Second blocks are unused
    for (size_t j = ITERATIONS; j < 2 * ITERATIONS; ++j)
    {
      size_t const block_size = blocks[j].first;
      size_t const block_used = blocks[j].second;
      REQUIRE_EQ(block_size, 2 * MEMORY_S);
      REQUIRE_EQ(block_used, 0);
    }
  }
}

/***/
TEST_CASE("allocate_deallocate_many")
{
  FreeListAllocatorMock fla;

  void* p1 = fla.allocate(64);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 64);
    REQUIRE_EQ(block_used, 1);
  }

  void* p2 = fla.allocate(64);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 2);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 2);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }
  }

  void* p3 = fla.allocate(512);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 3);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 512);
      REQUIRE_EQ(block_used, 1);
    }
  }

  fla.deallocate(p1);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 3);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 512);
      REQUIRE_EQ(block_used, 1);
    }
  }

  fla.deallocate(p2);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 3);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 512);
      REQUIRE_EQ(block_used, 1);
    }
  }

  fla.deallocate(p3);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 3);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 512);
      REQUIRE_EQ(block_used, 0);
    }
  }

  void* p4 = fla.allocate(256);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 3);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 224);
      REQUIRE_EQ(block_used, 0);
    }
  }

  fla.deallocate(p4);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 3);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 512);
      REQUIRE_EQ(block_used, 0);
    }
  }
}

/***/
TEST_CASE("block_too_small_to_split")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(64);

  // Check allocator internal state
  {
    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 0);

    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 64);
    REQUIRE_EQ(block_used, 1);
  }

  fla.deallocate(p);

  // Check allocator internal state
  {
    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    // Check freelist blocks
    size_t const block_size_fl = freelist_res[0].first;
    size_t const block_used_fl = freelist_res[0].second;
    REQUIRE_EQ(block_size_fl, 64);
    REQUIRE_EQ(block_used_fl, 0);

    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 64);
    REQUIRE_EQ(block_used, 0);
  }

  // Here we requested 32 bytes but it was not possible to split the block that previously
  // was 64 + BlockHeader so we return a bigger one instead of 64 bytes again
  void* p1 = fla.allocate(32);

  // Check allocator internal state
  {
    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 0);

    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 64);
    REQUIRE_EQ(block_used, 1);
  }

  void* p2 = fla.allocate(32);

  // Check allocator internal state
  {
    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 0);

    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block, we had no more space so we allocated more
    REQUIRE_EQ(total_allocated_blocks, 2);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 2);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 64);
    REQUIRE_EQ(block_used, 1);

    size_t const block_size2 = blocks[1].first;
    size_t const block_used2 = blocks[1].second;
    REQUIRE_EQ(block_size2, 32);
    REQUIRE_EQ(block_used2, 1);
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_next")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(128);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 0);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 128);
    REQUIRE_EQ(block_used, 1);
  }

  fla.deallocate(p);

  // Check allocator internal state
  {
    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    // Check freelist blocks
    size_t const block_size_fl = freelist_res[0].first;
    size_t const block_used_fl = freelist_res[0].second;
    REQUIRE_EQ(block_size_fl, 128);
    REQUIRE_EQ(block_used_fl, 0);

    // Check allocated blocks
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 128);
    REQUIRE_EQ(block_used, 0);
  }

  // we know that in total we have 128 + 32 (header size) = 160 bits

  // we can request two separate allocations without re-allocating more memory from the heap
  // 64 + 32 (header) = 96
  // 32 + 32 (header) = 64

  void* p1 = fla.allocate(64);

  // Check allocator internal state
  {
    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    // Check freelist blocks
    size_t const block_size_fl = freelist_res[0].first;
    size_t const block_used_fl = freelist_res[0].second;
    REQUIRE_EQ(block_size_fl, 32);
    REQUIRE_EQ(block_used_fl, 0);

    // Check allocated blocks
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 2);

    // The block should be split here
    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 64);
    REQUIRE_EQ(block_used, 1);

    size_t const block_size2 = blocks[1].first;
    size_t const block_used2 = blocks[1].second;
    REQUIRE_EQ(block_size2, 32);
    REQUIRE_EQ(block_used2, 0);
  }

  void* p2 = fla.allocate(32);

  // Check allocator internal state
  {
    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 0);

    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 2);

    // The block should be split here
    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 64);
    REQUIRE_EQ(block_used, 1);

    size_t const block_size2 = blocks[1].first;
    size_t const block_used2 = blocks[1].second;
    REQUIRE_EQ(block_size2, 32);
    REQUIRE_EQ(block_used2, 1);
  }

  // now deallocate first p2 and then p1 to coalesce_next

  fla.deallocate(p2);

  // Check allocator internal state
  {
    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    // Check freelist blocks
    size_t const block_size_fl = freelist_res[0].first;
    size_t const block_used_fl = freelist_res[0].second;
    REQUIRE_EQ(block_size_fl, 32);
    REQUIRE_EQ(block_used_fl, 0);

    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 2);

    // The block should be split here
    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 64);
    REQUIRE_EQ(block_used, 1);

    size_t const block_size2 = blocks[1].first;
    size_t const block_used2 = blocks[1].second;
    REQUIRE_EQ(block_size2, 32);
    REQUIRE_EQ(block_used2, 0);
  }

  fla.deallocate(p1);

  // Now blocks are both coalesced to a single block with a single header

  // Check allocator internal state
  {
    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    // Check freelist blocks
    size_t const block_size_fl = freelist_res[0].first;
    size_t const block_used_fl = freelist_res[0].second;
    REQUIRE_EQ(block_size_fl, 128);
    REQUIRE_EQ(block_used_fl, 0);

    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    // The block should be split here
    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 128);
    REQUIRE_EQ(block_used, 0);
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_previous")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(128);

  // Check allocator internal state
  {
    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 0);

    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 128);
    REQUIRE_EQ(block_used, 1);
  }

  fla.deallocate(p);

  // Check allocator internal state
  {
    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    // Check freelist blocks
    size_t const block_size_fl = freelist_res[0].first;
    size_t const block_used_fl = freelist_res[0].second;
    REQUIRE_EQ(block_size_fl, 128);
    REQUIRE_EQ(block_used_fl, 0);

    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 128);
    REQUIRE_EQ(block_used, 0);
  }

  // we know that in total we have 128 + 32 (header size) = 160 bits

  // we can request two separate allocations without re-allocating more memory from the heap
  // 64 + 32 (header) = 96
  // 32 + 32 (header) = 64

  void* p1 = fla.allocate(64);

  // Check allocator internal state
  {
    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    // Check freelist blocks
    size_t const block_size_fl = freelist_res[0].first;
    size_t const block_used_fl = freelist_res[0].second;
    REQUIRE_EQ(block_size_fl, 32);
    REQUIRE_EQ(block_used_fl, 0);

    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 2);

    // The block should be split here
    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 64);
    REQUIRE_EQ(block_used, 1);

    size_t const block_size2 = blocks[1].first;
    size_t const block_used2 = blocks[1].second;
    REQUIRE_EQ(block_size2, 32);
    REQUIRE_EQ(block_used2, 0);
  }

  void* p2 = fla.allocate(32);

  // Check allocator internal state
  {
    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 0);

    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 2);

    // The block should be split here
    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 64);
    REQUIRE_EQ(block_used, 1);

    size_t const block_size2 = blocks[1].first;
    size_t const block_used2 = blocks[1].second;
    REQUIRE_EQ(block_size2, 32);
    REQUIRE_EQ(block_used2, 1);
  }

  // now deallocate first p1 and then p2 to coalesce_previous
  fla.deallocate(p1);

  // Check allocator internal state
  {
    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    // Check freelist blocks
    size_t const block_size_fl = freelist_res[0].first;
    size_t const block_used_fl = freelist_res[0].second;
    REQUIRE_EQ(block_size_fl, 64);
    REQUIRE_EQ(block_used_fl, 0);

    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 2);

    // The block should be split here
    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 64);
    REQUIRE_EQ(block_used, 0);

    size_t const block_size2 = blocks[1].first;
    size_t const block_used2 = blocks[1].second;
    REQUIRE_EQ(block_size2, 32);
    REQUIRE_EQ(block_used2, 1);
  }

  fla.deallocate(p2);

  // Now blocks are both coalesced to a single block with a single header

  // Check allocator internal state
  {
    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    // Check freelist blocks
    size_t const block_size_fl = freelist_res[0].first;
    size_t const block_used_fl = freelist_res[0].second;
    REQUIRE_EQ(block_size_fl, 128);
    REQUIRE_EQ(block_used_fl, 0);

    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    // The block should be split here
    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 128);
    REQUIRE_EQ(block_used, 0);
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_previous_and_next_case_p1p2p3p4")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(640);
  fla.deallocate(p);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 640);
    REQUIRE_EQ(block_used, 0);

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    // Check freelist blocks
    size_t const block_size_fl = freelist_res[0].first;
    size_t const block_used_fl = freelist_res[0].second;
    REQUIRE_EQ(block_size_fl, 640);
    REQUIRE_EQ(block_used_fl, 0);
  }

  // Allocate 3 blocks of different size
  void* p1 = fla.allocate(128);
  void* p2 = fla.allocate(64);
  void* p3 = fla.allocate(256);
  void* p4 = fla.allocate(96);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }
  }

  // Now start deallocating
  // we will deallocate p1 and p4 first
  fla.deallocate(p1);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 128);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p2);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 224);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 224);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p3);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 2);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 512);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 512);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p4);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 640);
      REQUIRE_EQ(block_used, 0);
    }

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 640);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_previous_and_next_case_p1p2p4p3")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(640);
  fla.deallocate(p);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 640);
    REQUIRE_EQ(block_used, 0);

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 640);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  // Allocate 3 blocks of different size
  void* p1 = fla.allocate(128);
  void* p2 = fla.allocate(64);
  void* p3 = fla.allocate(256);
  void* p4 = fla.allocate(96);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }
  }

  // Now start deallocating
  fla.deallocate(p1);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 128);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p2);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 224);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 224);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p4);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 224);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 0);
    }

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 96);
      REQUIRE_EQ(block_used_fl, 0);
    }

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[1].first;
      size_t const block_used_fl = freelist_res[1].second;
      REQUIRE_EQ(block_size_fl, 224);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p3);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 640);
      REQUIRE_EQ(block_used, 0);
    }

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 640);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_previous_and_next_case_p1p4p2p3")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(640);
  fla.deallocate(p);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 640);
    REQUIRE_EQ(block_used, 0);
  }

  // Allocate 3 blocks of different size
  void* p1 = fla.allocate(128);
  void* p2 = fla.allocate(64);
  void* p3 = fla.allocate(256);
  void* p4 = fla.allocate(96);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }
  }

  // Now start deallocating
  fla.deallocate(p1);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 128);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p4);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 0);
    }

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 96);
      REQUIRE_EQ(block_used_fl, 0);
    }

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[1].first;
      size_t const block_used_fl = freelist_res[1].second;
      REQUIRE_EQ(block_size_fl, 128);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p2);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 224);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 0);
    }

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 96);
      REQUIRE_EQ(block_used_fl, 0);
    }

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[1].first;
      size_t const block_used_fl = freelist_res[1].second;
      REQUIRE_EQ(block_size_fl, 224);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p3);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 640);
      REQUIRE_EQ(block_used, 0);
    }

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 640);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_previous_and_next_case_p1p4p3p2")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(640);
  fla.deallocate(p);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 640);
    REQUIRE_EQ(block_used, 0);
  }

  // Allocate 3 blocks of different size
  void* p1 = fla.allocate(128);
  void* p2 = fla.allocate(64);
  void* p3 = fla.allocate(256);
  void* p4 = fla.allocate(96);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }
  }

  // Now start deallocating
  fla.deallocate(p1);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 128);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p4);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 0);
    }

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 96);
      REQUIRE_EQ(block_used_fl, 0);
    }

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[1].first;
      size_t const block_used_fl = freelist_res[1].second;
      REQUIRE_EQ(block_size_fl, 128);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p3);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 384);
      REQUIRE_EQ(block_used, 0);
    }

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 128);
      REQUIRE_EQ(block_used_fl, 0);
    }

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[1].first;
      size_t const block_used_fl = freelist_res[1].second;
      REQUIRE_EQ(block_size_fl, 384);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p2);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 640);
      REQUIRE_EQ(block_used, 0);
    }

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 640);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_previous_and_next_case_p1p3p2p4")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(640);
  fla.deallocate(p);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 640);
    REQUIRE_EQ(block_used, 0);
  }

  // Allocate 3 blocks of different size
  void* p1 = fla.allocate(128);
  void* p2 = fla.allocate(64);
  void* p3 = fla.allocate(256);
  void* p4 = fla.allocate(96);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }
  }

  // Now start deallocating
  fla.deallocate(p1);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 128);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p3);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 128);
      REQUIRE_EQ(block_used_fl, 0);
    }

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[1].first;
      size_t const block_used_fl = freelist_res[1].second;
      REQUIRE_EQ(block_size_fl, 256);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p2);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 2);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 512);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 512);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p4);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 640);
      REQUIRE_EQ(block_used, 0);
    }

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 640);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_previous_and_next_case_p1p3p4p2")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(640);
  fla.deallocate(p);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 640);
    REQUIRE_EQ(block_used, 0);
  }

  // Allocate 3 blocks of different size
  void* p1 = fla.allocate(128);
  void* p2 = fla.allocate(64);
  void* p3 = fla.allocate(256);
  void* p4 = fla.allocate(96);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }
  }

  fla.deallocate(p1);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 128);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p3);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 128);
      REQUIRE_EQ(block_used_fl, 0);
    }

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[1].first;
      size_t const block_used_fl = freelist_res[1].second;
      REQUIRE_EQ(block_size_fl, 256);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p4);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 384);
      REQUIRE_EQ(block_used, 0);
    }

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 128);
      REQUIRE_EQ(block_used_fl, 0);
    }

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[1].first;
      size_t const block_used_fl = freelist_res[1].second;
      REQUIRE_EQ(block_size_fl, 384);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p2);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 640);
      REQUIRE_EQ(block_used, 0);
    }

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 640);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_previous_and_next_case_p2p1p3p4")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(640);
  fla.deallocate(p);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 640);
    REQUIRE_EQ(block_used, 0);
  }

  // Allocate 3 blocks of different size
  void* p1 = fla.allocate(128);
  void* p2 = fla.allocate(64);
  void* p3 = fla.allocate(256);
  void* p4 = fla.allocate(96);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }
  }

  fla.deallocate(p2);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 64);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p1);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 224);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 224);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p3);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 2);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 512);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 512);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p4);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 640);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 640);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_previous_and_next_case_p2p1p4p3")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(640);
  fla.deallocate(p);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 640);
    REQUIRE_EQ(block_used, 0);
  }

  // Allocate 3 blocks of different size
  void* p1 = fla.allocate(128);
  void* p2 = fla.allocate(64);
  void* p3 = fla.allocate(256);
  void* p4 = fla.allocate(96);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }
  }

  fla.deallocate(p2);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 64);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p1);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 224);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 224);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p4);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 224);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 96);
      REQUIRE_EQ(block_used_fl, 0);
    }

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[1].first;
      size_t const block_used_fl = freelist_res[1].second;
      REQUIRE_EQ(block_size_fl, 224);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p3);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 640);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 640);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_previous_and_next_case_p2p3p1p4")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(640);
  fla.deallocate(p);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 640);
    REQUIRE_EQ(block_used, 0);
  }

  // Allocate 3 blocks of different size
  void* p1 = fla.allocate(128);
  void* p2 = fla.allocate(64);
  void* p3 = fla.allocate(256);
  void* p4 = fla.allocate(96);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }
  }

  fla.deallocate(p2);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 64);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p3);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 352);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 352);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p1);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 2);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 512);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 512);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p4);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 640);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 640);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_previous_and_next_case_p2p3p4p1")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(640);
  fla.deallocate(p);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 640);
    REQUIRE_EQ(block_used, 0);
  }

  // Allocate 3 blocks of different size
  void* p1 = fla.allocate(128);
  void* p2 = fla.allocate(64);
  void* p3 = fla.allocate(256);
  void* p4 = fla.allocate(96);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }
  }

  fla.deallocate(p2);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 64);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p3);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 352);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 352);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p4);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 2);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 480);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 480);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p1);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 640);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 640);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_previous_and_next_case_p2p4p1p3")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(640);
  fla.deallocate(p);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 640);
    REQUIRE_EQ(block_used, 0);
  }

  // Allocate 3 blocks of different size
  void* p1 = fla.allocate(128);
  void* p2 = fla.allocate(64);
  void* p3 = fla.allocate(256);
  void* p4 = fla.allocate(96);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }
  }

  fla.deallocate(p2);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 64);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p4);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 64);
      REQUIRE_EQ(block_used_fl, 0);
    }

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[1].first;
      size_t const block_used_fl = freelist_res[1].second;
      REQUIRE_EQ(block_size_fl, 96);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p1);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 224);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 96);
      REQUIRE_EQ(block_used_fl, 0);
    }

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[1].first;
      size_t const block_used_fl = freelist_res[1].second;
      REQUIRE_EQ(block_size_fl, 224);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p3);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 640);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 640);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_previous_and_next_case_p2p4p3p1")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(640);
  fla.deallocate(p);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 640);
    REQUIRE_EQ(block_used, 0);
  }

  // Allocate 3 blocks of different size
  void* p1 = fla.allocate(128);
  void* p2 = fla.allocate(64);
  void* p3 = fla.allocate(256);
  void* p4 = fla.allocate(96);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }
  }

  fla.deallocate(p2);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 64);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p4);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 64);
      REQUIRE_EQ(block_used_fl, 0);
    }

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[1].first;
      size_t const block_used_fl = freelist_res[1].second;
      REQUIRE_EQ(block_size_fl, 96);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p3);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 2);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 480);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 480);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p1);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 640);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 640);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_previous_and_next_case_p3p2p1p4")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(640);
  fla.deallocate(p);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 640);
    REQUIRE_EQ(block_used, 0);
  }

  // Allocate 3 blocks of different size
  void* p1 = fla.allocate(128);
  void* p2 = fla.allocate(64);
  void* p3 = fla.allocate(256);
  void* p4 = fla.allocate(96);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }
  }

  fla.deallocate(p3);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 256);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p2);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 352);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 352);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p1);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 2);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 512);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 512);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p4);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 640);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 640);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_previous_and_next_case_p3p2p4p1")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(640);
  fla.deallocate(p);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 640);
    REQUIRE_EQ(block_used, 0);
  }

  // Allocate 3 blocks of different size
  void* p1 = fla.allocate(128);
  void* p2 = fla.allocate(64);
  void* p3 = fla.allocate(256);
  void* p4 = fla.allocate(96);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }
  }

  fla.deallocate(p3);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 256);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p2);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 352);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 352);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p4);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 2);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 480);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 480);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p1);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 640);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 640);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_previous_and_next_case_p3p1p2p4")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(640);
  fla.deallocate(p);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 640);
    REQUIRE_EQ(block_used, 0);
  }

  // Allocate 3 blocks of different size
  void* p1 = fla.allocate(128);
  void* p2 = fla.allocate(64);
  void* p3 = fla.allocate(256);
  void* p4 = fla.allocate(96);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }
  }

  fla.deallocate(p3);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 256);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p1);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 128);
      REQUIRE_EQ(block_used_fl, 0);
    }

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[1].first;
      size_t const block_used_fl = freelist_res[1].second;
      REQUIRE_EQ(block_size_fl, 256);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p2);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 2);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 512);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 512);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p4);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 640);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 640);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_previous_and_next_case_p3p1p4p2")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(640);
  fla.deallocate(p);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 640);
    REQUIRE_EQ(block_used, 0);
  }

  // Allocate 3 blocks of different size
  void* p1 = fla.allocate(128);
  void* p2 = fla.allocate(64);
  void* p3 = fla.allocate(256);
  void* p4 = fla.allocate(96);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }
  }

  fla.deallocate(p3);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 256);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p1);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 128);
      REQUIRE_EQ(block_used_fl, 0);
    }

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[1].first;
      size_t const block_used_fl = freelist_res[1].second;
      REQUIRE_EQ(block_size_fl, 256);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p4);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 384);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 128);
      REQUIRE_EQ(block_used_fl, 0);
    }

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[1].first;
      size_t const block_used_fl = freelist_res[1].second;
      REQUIRE_EQ(block_size_fl, 384);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p2);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 640);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 640);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_previous_and_next_case_p3p4p1p2")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(640);
  fla.deallocate(p);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 640);
    REQUIRE_EQ(block_used, 0);
  }

  // Allocate 3 blocks of different size
  void* p1 = fla.allocate(128);
  void* p2 = fla.allocate(64);
  void* p3 = fla.allocate(256);
  void* p4 = fla.allocate(96);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }
  }

  fla.deallocate(p3);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 256);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p4);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 384);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 384);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p1);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 384);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 128);
      REQUIRE_EQ(block_used_fl, 0);
    }

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[1].first;
      size_t const block_used_fl = freelist_res[1].second;
      REQUIRE_EQ(block_size_fl, 384);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p2);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 640);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 640);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_previous_and_next_case_p3p4p2p1")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(640);
  fla.deallocate(p);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 640);
    REQUIRE_EQ(block_used, 0);
  }

  // Allocate 3 blocks of different size
  void* p1 = fla.allocate(128);
  void* p2 = fla.allocate(64);
  void* p3 = fla.allocate(256);
  void* p4 = fla.allocate(96);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }
  }

  fla.deallocate(p3);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 256);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p4);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 384);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 384);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p2);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 2);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 480);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 480);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p1);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 640);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 640);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_previous_and_next_case_p4p1p2p3")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(640);
  fla.deallocate(p);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 640);
    REQUIRE_EQ(block_used, 0);
  }

  // Allocate 3 blocks of different size
  void* p1 = fla.allocate(128);
  void* p2 = fla.allocate(64);
  void* p3 = fla.allocate(256);
  void* p4 = fla.allocate(96);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }
  }

  fla.deallocate(p4);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 96);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p1);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 96);
      REQUIRE_EQ(block_used_fl, 0);
    }

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[1].first;
      size_t const block_used_fl = freelist_res[1].second;
      REQUIRE_EQ(block_size_fl, 128);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p2);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 224);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 96);
      REQUIRE_EQ(block_used_fl, 0);
    }

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[1].first;
      size_t const block_used_fl = freelist_res[1].second;
      REQUIRE_EQ(block_size_fl, 224);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p3);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 640);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 640);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_previous_and_next_case_p4p1p3p2")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(640);
  fla.deallocate(p);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 640);
    REQUIRE_EQ(block_used, 0);
  }

  // Allocate 3 blocks of different size
  void* p1 = fla.allocate(128);
  void* p2 = fla.allocate(64);
  void* p3 = fla.allocate(256);
  void* p4 = fla.allocate(96);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }
  }

  fla.deallocate(p4);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 96);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p1);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 96);
      REQUIRE_EQ(block_used_fl, 0);
    }

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[1].first;
      size_t const block_used_fl = freelist_res[1].second;
      REQUIRE_EQ(block_size_fl, 128);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p3);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 384);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 128);
      REQUIRE_EQ(block_used_fl, 0);
    }

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[1].first;
      size_t const block_used_fl = freelist_res[1].second;
      REQUIRE_EQ(block_size_fl, 384);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p2);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 640);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 640);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_previous_and_next_case_p4p3p2p1")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(640);
  fla.deallocate(p);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 640);
    REQUIRE_EQ(block_used, 0);
  }

  // Allocate 3 blocks of different size
  void* p1 = fla.allocate(128);
  void* p2 = fla.allocate(64);
  void* p3 = fla.allocate(256);
  void* p4 = fla.allocate(96);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }
  }

  fla.deallocate(p4);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 96);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p3);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 384);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 384);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p2);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 2);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 480);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 480);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p1);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 640);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 640);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_previous_and_next_case_p4p3p1p2")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(640);
  fla.deallocate(p);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 640);
    REQUIRE_EQ(block_used, 0);
  }

  // Allocate 3 blocks of different size
  void* p1 = fla.allocate(128);
  void* p2 = fla.allocate(64);
  void* p3 = fla.allocate(256);
  void* p4 = fla.allocate(96);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }
  }

  fla.deallocate(p4);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 96);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p3);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 384);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 384);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p1);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 384);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 128);
      REQUIRE_EQ(block_used_fl, 0);
    }

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[1].first;
      size_t const block_used_fl = freelist_res[1].second;
      REQUIRE_EQ(block_size_fl, 384);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p2);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 640);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 640);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_previous_and_next_case_p4p2p3p1")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(640);
  fla.deallocate(p);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 640);
    REQUIRE_EQ(block_used, 0);
  }

  // Allocate 3 blocks of different size
  void* p1 = fla.allocate(128);
  void* p2 = fla.allocate(64);
  void* p3 = fla.allocate(256);
  void* p4 = fla.allocate(96);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }
  }

  fla.deallocate(p4);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 96);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p2);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 64);
      REQUIRE_EQ(block_used_fl, 0);
    }

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[1].first;
      size_t const block_used_fl = freelist_res[1].second;
      REQUIRE_EQ(block_size_fl, 96);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p3);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 2);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 480);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 480);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p1);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 640);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 640);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }
}

/***/
TEST_CASE("block_enough_to_split_coalesce_previous_and_next_case_p4p2p1p3")
{
  FreeListAllocatorMock fla;

  void* p = fla.allocate(640);
  fla.deallocate(p);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, 640);
    REQUIRE_EQ(block_used, 0);
  }

  // Allocate 3 blocks of different size
  void* p1 = fla.allocate(128);
  void* p2 = fla.allocate(64);
  void* p3 = fla.allocate(256);
  void* p4 = fla.allocate(96);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 1);
    }
  }

  fla.deallocate(p4);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 96);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p2);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 4);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 128);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 64);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[3].first;
      size_t const block_used = blocks[3].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 64);
      REQUIRE_EQ(block_used_fl, 0);
    }

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[1].first;
      size_t const block_used_fl = freelist_res[1].second;
      REQUIRE_EQ(block_size_fl, 96);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p1);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 3);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 224);
      REQUIRE_EQ(block_used, 0);
    }

    {
      size_t const block_size = blocks[1].first;
      size_t const block_used = blocks[1].second;
      REQUIRE_EQ(block_size, 256);
      REQUIRE_EQ(block_used, 1);
    }

    {
      size_t const block_size = blocks[2].first;
      size_t const block_used = blocks[2].second;
      REQUIRE_EQ(block_size, 96);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 96);
      REQUIRE_EQ(block_used_fl, 0);
    }
    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[1].first;
      size_t const block_used_fl = freelist_res[1].second;
      REQUIRE_EQ(block_size_fl, 224);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }

  fla.deallocate(p3);

  // Check allocator internal state
  {
    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // Only one allocated block
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check all allocated blocks
    REQUIRE_EQ(blocks.size(), 1);

    {
      size_t const block_size = blocks[0].first;
      size_t const block_used = blocks[0].second;
      REQUIRE_EQ(block_size, 640);
      REQUIRE_EQ(block_used, 0);
    }

    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    {
      // Check freelist blocks
      size_t const block_size_fl = freelist_res[0].first;
      size_t const block_used_fl = freelist_res[0].second;
      REQUIRE_EQ(block_size_fl, 640);
      REQUIRE_EQ(block_used_fl, 0);
    }
  }
}

/***/
TEST_CASE("allocate_slice_then_allocate_more_with_minimum_allocation")
{
  constexpr size_t TOTAL_CAPACITY{4'096};
  FreeListAllocatorMock fla{TOTAL_CAPACITY};

  fla.set_minimum_allocation(TOTAL_CAPACITY);

  {
    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    // Check freelist blocks
    size_t const block_size_fl = freelist_res[0].first;
    size_t const block_used_fl = freelist_res[0].second;
    REQUIRE_EQ(block_size_fl, TOTAL_CAPACITY);
    REQUIRE_EQ(block_used_fl, 0);

    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // we only expect 1 allocation
    REQUIRE_EQ(total_allocated_blocks, 1);

    // Check block
    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, TOTAL_CAPACITY);
    REQUIRE_EQ(block_used, 0);
  }

  // Allocate 1024, slicing the block
  constexpr size_t CHUNK_SIZE{1'024};
  void* p1 = fla.allocate(CHUNK_SIZE);

  {
    const size_t free_memory = TOTAL_CAPACITY - (CHUNK_SIZE + fla.size_of_header());

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    // Check freelist blocks
    size_t const block_size_fl = freelist_res[0].first;
    size_t const block_used_fl = freelist_res[0].second;
    REQUIRE_EQ(block_size_fl, free_memory);
    REQUIRE_EQ(block_used_fl, 0);

    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // we only expect 1 allocation
    REQUIRE_EQ(total_allocated_blocks, 1);

    REQUIRE_EQ(blocks.size(), 2);

    // Check block
    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, CHUNK_SIZE);
    REQUIRE_EQ(block_used, 1);

    // Check block
    size_t const block_size2 = blocks[1].first;
    size_t const block_used2 = blocks[1].second;
    REQUIRE_EQ(block_size2, free_memory);
    REQUIRE_EQ(block_used2, 0);
  }

  // Allocate 1024, slicing the block
  void* p2 = fla.allocate(CHUNK_SIZE);

  {
    const size_t free_memory = TOTAL_CAPACITY - 2 * (CHUNK_SIZE + fla.size_of_header());

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    // Check freelist blocks
    size_t const block_size_fl = freelist_res[0].first;
    size_t const block_used_fl = freelist_res[0].second;
    REQUIRE_EQ(block_size_fl, free_memory);
    REQUIRE_EQ(block_used_fl, 0);

    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // we only expect 1 allocation
    REQUIRE_EQ(total_allocated_blocks, 1);

    REQUIRE_EQ(blocks.size(), 3);

    // Check block
    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, CHUNK_SIZE);
    REQUIRE_EQ(block_used, 1);

    size_t const block_size2 = blocks[1].first;
    size_t const block_used2 = blocks[1].second;
    REQUIRE_EQ(block_size2, CHUNK_SIZE);
    REQUIRE_EQ(block_used2, 1);

    // Check block
    size_t const block_size3 = blocks[2].first;
    size_t const block_used3 = blocks[2].second;
    REQUIRE_EQ(block_size3, free_memory);
    REQUIRE_EQ(block_used3, 0);
  }

  // Allocate 1024, slicing the block
  void* p3 = fla.allocate(CHUNK_SIZE);

  {
    const size_t free_memory = TOTAL_CAPACITY - 3 * (CHUNK_SIZE + fla.size_of_header());

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 1);

    // Check freelist blocks
    size_t const block_size_fl = freelist_res[0].first;
    size_t const block_used_fl = freelist_res[0].second;
    REQUIRE_EQ(block_size_fl, free_memory);
    REQUIRE_EQ(block_used_fl, 0);

    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // we only expect 1 allocation
    REQUIRE_EQ(total_allocated_blocks, 1);

    REQUIRE_EQ(blocks.size(), 4);

    // Check block
    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, CHUNK_SIZE);
    REQUIRE_EQ(block_used, 1);

    size_t const block_size2 = blocks[1].first;
    size_t const block_used2 = blocks[1].second;
    REQUIRE_EQ(block_size2, CHUNK_SIZE);
    REQUIRE_EQ(block_used2, 1);

    size_t const block_size3 = blocks[2].first;
    size_t const block_used3 = blocks[2].second;
    REQUIRE_EQ(block_size3, CHUNK_SIZE);
    REQUIRE_EQ(block_used3, 1);

    // Check block
    size_t const block_size4 = blocks[3].first;
    size_t const block_used4 = blocks[3].second;
    REQUIRE_EQ(block_size4, free_memory);
    REQUIRE_EQ(block_used4, 0);
  }

  // Now that we have 4 slices - 3 taken and 1 free we will request more
  // memory from the allcoate forcing to to malloc and add a new block
  constexpr size_t NEW_CAPACITY{2'048};
  void* p4 = fla.allocate(NEW_CAPACITY);

  {
    const size_t free_memory = TOTAL_CAPACITY - 3 * (CHUNK_SIZE + fla.size_of_header());

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    // Check freelist blocks
    size_t const block_size_fl = freelist_res[0].first;
    size_t const block_used_fl = freelist_res[0].second;
    REQUIRE_EQ(block_size_fl, free_memory);
    REQUIRE_EQ(block_used_fl, 0);

    size_t const block_size_fl1 = freelist_res[1].first;
    size_t const block_used_fl1 = freelist_res[1].second;
    REQUIRE_EQ(block_size_fl1, 2016);
    REQUIRE_EQ(block_used_fl1, 0);

    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // we only expect 1 allocation
    REQUIRE_EQ(total_allocated_blocks, 2);

    REQUIRE_EQ(blocks.size(), 6);

    // Check block
    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, CHUNK_SIZE);
    REQUIRE_EQ(block_used, 1);

    size_t const block_size2 = blocks[1].first;
    size_t const block_used2 = blocks[1].second;
    REQUIRE_EQ(block_size2, CHUNK_SIZE);
    REQUIRE_EQ(block_used2, 1);

    size_t const block_size3 = blocks[2].first;
    size_t const block_used3 = blocks[2].second;
    REQUIRE_EQ(block_size3, CHUNK_SIZE);
    REQUIRE_EQ(block_used3, 1);

    // Check block
    size_t const block_size4 = blocks[3].first;
    size_t const block_used4 = blocks[3].second;
    REQUIRE_EQ(block_size4, free_memory);
    REQUIRE_EQ(block_used4, 0);

    size_t const block_size5 = blocks[4].first;
    size_t const block_used5 = blocks[4].second;
    REQUIRE_EQ(block_size5, NEW_CAPACITY);
    REQUIRE_EQ(block_used5, 1);

    size_t const block_size6 = blocks[5].first;
    size_t const block_used6 = blocks[5].second;
    REQUIRE_EQ(block_size6, 2016);
    REQUIRE_EQ(block_used6, 0);

    // check blocks in reverse order
    auto const results_reversed = fla.get_all_blocks_reverse();
    size_t const total_allocated_blocks_reversed = results_reversed.second;
    auto const& blocks_reversed = results_reversed.first;

    // we only expect 1 allocation
    REQUIRE_EQ(total_allocated_blocks_reversed, 2);

    REQUIRE_EQ(blocks_reversed.size(), 6);

    // Check block
    size_t const block_size_reversed0 = blocks_reversed[0].first;
    size_t const block_used_reversed0 = blocks_reversed[0].second;
    REQUIRE_EQ(block_size_reversed0, 2016);
    REQUIRE_EQ(block_used_reversed0, 0);

    size_t const block_size_reversed = blocks_reversed[1].first;
    size_t const block_used_reversed = blocks_reversed[1].second;
    REQUIRE_EQ(block_size_reversed, NEW_CAPACITY);
    REQUIRE_EQ(block_used_reversed, 1);

    size_t const block_size_reversed2 = blocks_reversed[2].first;
    size_t const block_used_reversed2 = blocks_reversed[2].second;
    REQUIRE_EQ(block_size_reversed2, free_memory);
    REQUIRE_EQ(block_used_reversed2, 0);

    size_t const block_size_reversed3 = blocks_reversed[3].first;
    size_t const block_used_reversed3 = blocks_reversed[3].second;
    REQUIRE_EQ(block_size_reversed3, CHUNK_SIZE);
    REQUIRE_EQ(block_used_reversed3, 1);

    // Check block
    size_t const block_size_reversed4 = blocks_reversed[4].first;
    size_t const block_used_reversed4 = blocks_reversed[4].second;
    REQUIRE_EQ(block_size_reversed4, CHUNK_SIZE);
    REQUIRE_EQ(block_used_reversed4, 1);

    size_t const block_size_reversed5 = blocks_reversed[5].first;
    size_t const block_used_reversed5 = blocks_reversed[5].second;
    REQUIRE_EQ(block_size_reversed5, CHUNK_SIZE);
    REQUIRE_EQ(block_used_reversed5, 1);
  }

  // Add one more allocation to request from OS
  // Now that we have 4 slices - 3 taken and 1 free we will request more
  // memory from the allcoate forcing to to malloc and add a new block
  void* p5 = fla.allocate(NEW_CAPACITY);

  {
    const size_t free_memory = TOTAL_CAPACITY - 3 * (CHUNK_SIZE + fla.size_of_header());

    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 3);

    // Check freelist blocks
    size_t const block_size_fl = freelist_res[0].first;
    size_t const block_used_fl = freelist_res[0].second;
    REQUIRE_EQ(block_size_fl, free_memory);
    REQUIRE_EQ(block_used_fl, 0);

    size_t const block_size_fl1 = freelist_res[1].first;
    size_t const block_used_fl1 = freelist_res[1].second;
    REQUIRE_EQ(block_size_fl1, 2016);
    REQUIRE_EQ(block_used_fl1, 0);

    size_t const block_size_fl2 = freelist_res[2].first;
    size_t const block_used_fl2 = freelist_res[2].second;
    REQUIRE_EQ(block_size_fl2, 2016);
    REQUIRE_EQ(block_used_fl2, 0);

    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // we only expect 1 allocation
    REQUIRE_EQ(total_allocated_blocks, 3);

    REQUIRE_EQ(blocks.size(), 8);

    // Check block
    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, CHUNK_SIZE);
    REQUIRE_EQ(block_used, 1);

    size_t const block_size2 = blocks[1].first;
    size_t const block_used2 = blocks[1].second;
    REQUIRE_EQ(block_size2, CHUNK_SIZE);
    REQUIRE_EQ(block_used2, 1);

    size_t const block_size3 = blocks[2].first;
    size_t const block_used3 = blocks[2].second;
    REQUIRE_EQ(block_size3, CHUNK_SIZE);
    REQUIRE_EQ(block_used3, 1);

    // Check block
    size_t const block_size4 = blocks[3].first;
    size_t const block_used4 = blocks[3].second;
    REQUIRE_EQ(block_size4, free_memory);
    REQUIRE_EQ(block_used4, 0);

    size_t const block_size5 = blocks[4].first;
    size_t const block_used5 = blocks[4].second;
    REQUIRE_EQ(block_size5, NEW_CAPACITY);
    REQUIRE_EQ(block_used5, 1);

    size_t const block_size6 = blocks[5].first;
    size_t const block_used6 = blocks[5].second;
    REQUIRE_EQ(block_size6, 2016);
    REQUIRE_EQ(block_used6, 0);

    size_t const block_size7 = blocks[6].first;
    size_t const block_used7 = blocks[6].second;
    REQUIRE_EQ(block_size7, NEW_CAPACITY);
    REQUIRE_EQ(block_used7, 1);

    size_t const block_size8 = blocks[7].first;
    size_t const block_used8 = blocks[7].second;
    REQUIRE_EQ(block_size8, 2016);
    REQUIRE_EQ(block_used8, 0);

    // check blocks in reverse order
    auto const results_reversed = fla.get_all_blocks_reverse();
    size_t const total_allocated_blocks_reversed = results_reversed.second;
    auto const& blocks_reversed = results_reversed.first;

    // we only expect 1 allocation
    REQUIRE_EQ(total_allocated_blocks_reversed, 3);

    REQUIRE_EQ(blocks_reversed.size(), 8);

    // Check block
    size_t const block_size_reversed00 = blocks_reversed[0].first;
    size_t const block_used_reversed00 = blocks_reversed[0].second;
    REQUIRE_EQ(block_size_reversed00, 2016);
    REQUIRE_EQ(block_used_reversed00, 0);

    size_t const block_size_reversed0 = blocks_reversed[1].first;
    size_t const block_used_reversed0 = blocks_reversed[1].second;
    REQUIRE_EQ(block_size_reversed0, NEW_CAPACITY);
    REQUIRE_EQ(block_used_reversed0, 1);

    size_t const block_size_reversed2 = blocks_reversed[2].first;
    size_t const block_used_reversed2 = blocks_reversed[2].second;
    REQUIRE_EQ(block_size_reversed2, 2016);
    REQUIRE_EQ(block_used_reversed2, 0);

    size_t const block_size_reversed21 = blocks_reversed[3].first;
    size_t const block_used_reversed21 = blocks_reversed[3].second;
    REQUIRE_EQ(block_size_reversed21, NEW_CAPACITY);
    REQUIRE_EQ(block_used_reversed21, 1);

    size_t const block_size_reversed3 = blocks_reversed[4].first;
    size_t const block_used_reversed3 = blocks_reversed[4].second;
    REQUIRE_EQ(block_size_reversed3, free_memory);
    REQUIRE_EQ(block_used_reversed3, 0);

    size_t const block_size_reversed4 = blocks_reversed[5].first;
    size_t const block_used_reversed4 = blocks_reversed[5].second;
    REQUIRE_EQ(block_size_reversed4, CHUNK_SIZE);
    REQUIRE_EQ(block_used_reversed4, 1);

    size_t const block_size_reversed5 = blocks_reversed[6].first;
    size_t const block_used_reversed5 = blocks_reversed[6].second;
    REQUIRE_EQ(block_size_reversed5, CHUNK_SIZE);
    REQUIRE_EQ(block_used_reversed5, 1);

    size_t const block_size_reversed6 = blocks_reversed[7].first;
    size_t const block_used_reversed6 = blocks_reversed[7].second;
    REQUIRE_EQ(block_size_reversed6, CHUNK_SIZE);
    REQUIRE_EQ(block_used_reversed6, 1);
  }

  // Do one more allocation for 1024 and expect to fill the empty block
  const size_t free_memory_from_initial_block = TOTAL_CAPACITY - 3 * (CHUNK_SIZE + fla.size_of_header()); // 0
  void* p6 = fla.allocate(free_memory_from_initial_block);

  {
    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 2);

    // Check freelist blocks
    size_t const block_size_fl1 = freelist_res[0].first;
    size_t const block_used_fl1 = freelist_res[0].second;
    REQUIRE_EQ(block_size_fl1, 2016);
    REQUIRE_EQ(block_used_fl1, 0);

    size_t const block_size_fl2 = freelist_res[1].first;
    size_t const block_used_fl2 = freelist_res[1].second;
    REQUIRE_EQ(block_size_fl2, 2016);
    REQUIRE_EQ(block_used_fl2, 0);

    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // we only expect 1 allocation
    REQUIRE_EQ(total_allocated_blocks, 3);

    REQUIRE_EQ(blocks.size(), 8);

    // Check block
    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, CHUNK_SIZE);
    REQUIRE_EQ(block_used, 1);

    size_t const block_size2 = blocks[1].first;
    size_t const block_used2 = blocks[1].second;
    REQUIRE_EQ(block_size2, CHUNK_SIZE);
    REQUIRE_EQ(block_used2, 1);

    size_t const block_size3 = blocks[2].first;
    size_t const block_used3 = blocks[2].second;
    REQUIRE_EQ(block_size3, CHUNK_SIZE);
    REQUIRE_EQ(block_used3, 1);

    // Check block
    size_t const block_size4 = blocks[3].first;
    size_t const block_used4 = blocks[3].second;
    REQUIRE_EQ(block_size4, free_memory_from_initial_block);
    REQUIRE_EQ(block_used4, 1);

    size_t const block_size5 = blocks[4].first;
    size_t const block_used5 = blocks[4].second;
    REQUIRE_EQ(block_size5, NEW_CAPACITY);
    REQUIRE_EQ(block_used5, 1);

    size_t const block_size6 = blocks[5].first;
    size_t const block_used6 = blocks[5].second;
    REQUIRE_EQ(block_size6, 2016);
    REQUIRE_EQ(block_used6, 0);

    size_t const block_size7 = blocks[6].first;
    size_t const block_used7 = blocks[6].second;
    REQUIRE_EQ(block_size7, NEW_CAPACITY);
    REQUIRE_EQ(block_used7, 1);

    size_t const block_size8 = blocks[7].first;
    size_t const block_used8 = blocks[7].second;
    REQUIRE_EQ(block_size8, 2016);
    REQUIRE_EQ(block_used8, 0);

    // check blocks in reverse order
    auto const results_reversed = fla.get_all_blocks_reverse();
    size_t const total_allocated_blocks_reversed = results_reversed.second;
    auto const& blocks_reversed = results_reversed.first;

    // we only expect 1 allocation
    REQUIRE_EQ(total_allocated_blocks_reversed, 3);

    REQUIRE_EQ(blocks_reversed.size(), 8);

    // Check block
    size_t const block_size_reversed00 = blocks_reversed[0].first;
    size_t const block_used_reversed00 = blocks_reversed[0].second;
    REQUIRE_EQ(block_size_reversed00, 2016);
    REQUIRE_EQ(block_used_reversed00, 0);

    size_t const block_size_reversed0 = blocks_reversed[1].first;
    size_t const block_used_reversed0 = blocks_reversed[1].second;
    REQUIRE_EQ(block_size_reversed0, NEW_CAPACITY);
    REQUIRE_EQ(block_used_reversed0, 1);

    size_t const block_size_reversed2 = blocks_reversed[2].first;
    size_t const block_used_reversed2 = blocks_reversed[2].second;
    REQUIRE_EQ(block_size_reversed2, 2016);
    REQUIRE_EQ(block_used_reversed2, 0);

    size_t const block_size_reversed21 = blocks_reversed[3].first;
    size_t const block_used_reversed21 = blocks_reversed[3].second;
    REQUIRE_EQ(block_size_reversed21, NEW_CAPACITY);
    REQUIRE_EQ(block_used_reversed21, 1);

    size_t const block_size_reversed3 = blocks_reversed[4].first;
    size_t const block_used_reversed3 = blocks_reversed[4].second;
    REQUIRE_EQ(block_size_reversed3, free_memory_from_initial_block);
    REQUIRE_EQ(block_used_reversed3, 1);

    size_t const block_size_reversed4 = blocks_reversed[5].first;
    size_t const block_used_reversed4 = blocks_reversed[5].second;
    REQUIRE_EQ(block_size_reversed4, CHUNK_SIZE);
    REQUIRE_EQ(block_used_reversed4, 1);

    size_t const block_size_reversed5 = blocks_reversed[6].first;
    size_t const block_used_reversed5 = blocks_reversed[6].second;
    REQUIRE_EQ(block_size_reversed5, CHUNK_SIZE);
    REQUIRE_EQ(block_used_reversed5, 1);

    size_t const block_size_reversed6 = blocks_reversed[7].first;
    size_t const block_used_reversed6 = blocks_reversed[7].second;
    REQUIRE_EQ(block_size_reversed6, CHUNK_SIZE);
    REQUIRE_EQ(block_used_reversed6, 1);
  }

  fla.deallocate(p1);
  fla.deallocate(p2);
  fla.deallocate(p3);
  fla.deallocate(p4);
  fla.deallocate(p5);
  fla.deallocate(p6);

  {
    // Check freelist blocks
    auto const freelist_res = fla.get_freelist();
    REQUIRE_EQ(freelist_res.size(), 3);

    // Check freelist blocks
    size_t const block_size_fl = freelist_res[0].first;
    size_t const block_used_fl = freelist_res[0].second;
    REQUIRE_EQ(block_size_fl, TOTAL_CAPACITY);
    REQUIRE_EQ(block_used_fl, 0);

    auto const results = fla.get_all_blocks();
    size_t const total_allocated_blocks = results.second;
    auto const& blocks = results.first;

    // we only expect 1 allocation
    REQUIRE_EQ(total_allocated_blocks, 3);

    // Check block
    size_t const block_size = blocks[0].first;
    size_t const block_used = blocks[0].second;
    REQUIRE_EQ(block_size, TOTAL_CAPACITY);
    REQUIRE_EQ(block_used, 0);

    size_t const block_size2 = blocks[1].first;
    size_t const block_used2 = blocks[1].second;
    REQUIRE_EQ(block_size2, TOTAL_CAPACITY);
    REQUIRE_EQ(block_used2, 0);

    size_t const block_size3 = blocks[2].first;
    size_t const block_used3 = blocks[2].second;
    REQUIRE_EQ(block_size3, TOTAL_CAPACITY);
    REQUIRE_EQ(block_used3, 0);
  }
}

TEST_SUITE_END();