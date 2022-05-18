/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Attributes.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace quill::detail
{

/**
 * The FreeListAllocator class requests memory from the OS and then when that memory is deallocated
 * is kept internally in a free list and can be later reused without the need to request new
 * memory from the operating system.
 *
 * Each block of memory has one or more Blocks with a BlockHeader.
 * A big block can be sliced to smaller blocks
 *
 * e.g.
 * 1) Initial allocation of 128 bytes
 *
 * | BlockHeader 32 bytes, used = true | Data 128 |
 * When this block gets deallocated the same structure is maintained.
 *
 * 2) A new allocation of 32 bytes is requested of the above free block.
 * The initial block is sliced. From the Data 128 size we use for the requested
 * payload 32 and then we allocate a new BlockHeader 32 bytes. 64 bytes is the remaining free memory
 * | BlockHeader 32 bytes, used = true | Data 32 | BlockHeader 32 bytes, used = false | Data 64 |
 *
 * The BlockHeader links to the next block of memory and to the previous block of memory regardless
 * whether they are used or free. We use the next* and previous* to coalesce during the free operation
 *
 * In addition to the above list, we also keep a collection of the free nodes. This is used during
 * allocation to speed up the process as we don't need to traverse the full list.
 *
 * If our freelist is empty, we request a new block of memory from the os
 */
class FreeListAllocator
{
public:
  /**
   * Default Constructor the free list allocator has zero capacity
   */
  FreeListAllocator();

  /**
   * Creates a free list allocator with default capacity.
   * If more memory that the initial capacity is requested the free list allocator automatically
   * requests more memory from the operating system.
   * Initial capacity is automatically rounded to be a multiple of std::max_align_t
   * @param initial_capacity the initial capacity
   */
  explicit FreeListAllocator(size_t initial_capacity);

  ~FreeListAllocator();

  /**
   * Deleted
   */
  FreeListAllocator(FreeListAllocator const&) = delete;
  FreeListAllocator& operator=(FreeListAllocator const&) = delete;

  /**
   * Reserves some initial capacity to the allocator.
   * This is meant to be called once in the beginning.
   * @param new_capacity the initial capacity
   */
  void reserve(size_t new_capacity);

  /**
   * Set a minimum allocation size.
   * By default this is zero, but it is recommended to set to the page size.
   * Must be a power of two.
   * @param minimum_alloc_size the minimum alloc size
   */
  void set_minimum_allocation(size_t minimum_alloc_size);

  /**
   * Allocates memory of size s
   * @param s the size of the allocated memory
   * @return a pointer to the allocated memory
   */
  QUILL_NODISCARD void* allocate(size_t s);

  /**
   * Frees the previously allocated block.
   * @param p the pointer to deallocate
   */
  void deallocate(void* p);

protected:
  /**
   * It is very hard to test this class without exposing some internal state so
   * we keep those variables protected to access them in tests.
   */
  /** Forward declaration */
  struct Block;

  /**
   * The header of the block.
   * We want to force the BlockHeader to be always 32 bytes. This is because even with
   * max_align_t == 8 we don't have enough size to be 2 * sizeof(max_align_t)
   */
  struct alignas(32) BlockHeader
  {
    /** The block size (data), excluding the size of the header. */
    size_t size{sizeof(uintptr_t)};

    /** Whether this block is allocated. */
    uint32_t used{0};

    /** A unique id each time we call malloc to allocate this block - All split blocks share the same id */
    uint32_t allocation_id{0};

    /** Next block. */
    Block* next{nullptr};

    /**  Previous block. */
    Block* previous{nullptr};
  };

  /**
   * Allocated block of memory.
   * Contains the object header structure, and the actual payload pointer.
   * Note here that sizeof(Block) will be 48 bytes for 64-bit as the compiler is adding
   * padding due to max_align_t
   */
  struct Block
  {
    BlockHeader header{};
    uintptr_t data[1]{};
  };

  static_assert(
    std::is_trivially_destructible<Block>::value,
    "Block is trivially destructible to avoid calling it's destructor after emplacement new");

private:
  /**
   * Tries to find a block that fits in our existing memory
   * @param size the requested size
   * @return a free block or nullptr if no block is in the freelist
   */
  QUILL_NODISCARD Block* _find_free_block(size_t size);

  /**
   * Slices the block to two blocks if there is enough space in the original block
   * @param block The original block to slice if possible
   * @param requested_size the requested_size by the user
   * @returns the pointer to the smaller sub-block, or the pointer to the original block if
   * slicing was not possible
   */
  QUILL_NODISCARD Block* _slice(Block* block, size_t requested_size);

  /**
   * Joins this block with the next
   * @param block the existing block to coalesce with it's next
   * @return the coalesced block
   */
  QUILL_NODISCARD Block* _coalesce_with_next(Block* block);

  /**
   * Joins this block with the previous
   * @param block the existing block to coalesce with it's previous
   * @return the coalesced block
   */
  QUILL_NODISCARD Block* _coalesce_with_previous(Block* block);

  /**
   * Requests (maps) memory from OS.
   * @param size the requested size by the user
   */
  QUILL_NODISCARD Block* _request_from_os(size_t size);

  /**
   * Request a free vector from the empty vector cache.
   * @return a pointer to an empty vector
   */
  QUILL_NODISCARD std::unique_ptr<std::vector<Block*>> _get_cached_vector();

  /**
   * Store an empty vector back to our empty vector cache
   * @param vec an empty vector
   */
  void _store_cached_vector(std::unique_ptr<std::vector<Block*>> vec);

  /**
   * We always round up the size to the max align value.
   * The BlockHeader is already 2 * sizeof(max_align_t).
   * Here we add a multiple of max_align_t for the actual size.
   * This means that we are always aligned to max_align_t size for all data* we return.
   * @param s the size we want to round
   * @return the size rounded to max_align_t, e.g 16,32,48,64 etc
   */
  QUILL_NODISCARD static size_t _add_padding(size_t s) noexcept;

  /**
   * Returns total allocation size, reserving in addition the space for
   * the BlockHeader structure
   * @param s the initial size
   * @return the size including the header
   */
  QUILL_NODISCARD static size_t _add_header_size(size_t s) noexcept;

protected:
  /**
   * Cached free vectors to return to the free list below.
   * When a vector is empty we want to remove it from the _free_list but we don't
   * want to lose we memory we allocated for this vector.
   */
  std::vector<std::unique_ptr<std::vector<Block*>>> _vector_cache;

  /**
   * Maps a size to a vector of free blocks
   */
  using size_blocks_pair_t = std::pair<size_t, std::unique_ptr<std::vector<Block*>>>;

  /**
   * Free list structure. Blocks are added to the free list on the `free` operation.
   * Consequent allocations of the appropriate size reuse the freed blocks.
   * If a std::map or std::multimap is used they call operator::new every time
   * an insertion happens to allocate a free node, so a std::vector is used instead.
   */
  std::vector<size_blocks_pair_t> _free_list;

  /**
   * We store all memory we have allocated here to free it later
   */
  std::vector<Block*> _requested_from_os;

  /**
   * Minimum requested allocation on allocate
   */
  size_t _minimum_allocation{0};
};

/**
 * A free list allocator custom deleter, that can be used with unique_ptrs
 */
template <typename T>
struct FreeListAllocatorDeleter
{
  FreeListAllocatorDeleter() = default;
  explicit FreeListAllocatorDeleter(FreeListAllocator* fla) : _fla(fla){};

  template <typename U = T>
  std::enable_if_t<std::is_trivially_destructible<U>::value> operator()(T* p)
  {
    _fla->deallocate(p);
  }

  template <typename U = T>
  std::enable_if_t<!std::is_trivially_destructible<U>::value> operator()(T* p)
  {
    p->~U();
    _fla->deallocate(p);
  }

  FreeListAllocator* _fla{nullptr};
};
} // namespace quill::detail