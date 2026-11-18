#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Common.h"
#include "quill/core/MathUtilities.h"
#include "quill/core/QuillError.h"

#ifndef QUILL_MODULE
  #include <atomic>
  #include <cerrno>
  #include <cstddef>
  #include <cstdint>
  #include <cstring>
  #include <new>
  #include <string>
#endif

#if defined(_WIN32)
  #include <malloc.h>
#else
  #include <sys/mman.h>
  #if defined(__linux__)
    // MAP_HUGE_2MB can be missing from older libc headers; construct it from the kernel ABI
    // values (page size log2, shifted by MAP_HUGE_SHIFT)
    #if !defined(MAP_HUGE_SHIFT)
      #define MAP_HUGE_SHIFT 26
    #endif
    #if !defined(MAP_HUGE_2MB)
      #define MAP_HUGE_2MB (21 << MAP_HUGE_SHIFT)
    #endif
  #endif
#endif

QUILL_BEGIN_NAMESPACE

namespace detail
{

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
  #pragma warning(push)
  #pragma warning(disable : 4324)
#endif

/**
 * A bounded single producer single consumer ring buffer.
 */
template <typename T>
class BoundedSPSCQueueImpl
{
public:
  using integer_type = T;

  static_assert(integer_type{0} < static_cast<integer_type>(~integer_type{0}),
                "BoundedSPSCQueueImpl integer_type must be unsigned");

  struct WriteReservation
  {
    std::byte* write_buffer{nullptr};
    integer_type writer_pos{0};
    BoundedSPSCQueueImpl* bounded_queue{nullptr};
  };

  /**
   * @throws QuillError if the capacity is too large or the storage cannot be allocated
   */
  QUILL_ATTRIBUTE_HOT explicit BoundedSPSCQueueImpl(integer_type capacity,
                                                    HugePagesPolicy huge_pages_policy = HugePagesPolicy::Never,
                                                    integer_type reader_store_percent = 5)
    : BoundedSPSCQueueImpl(std::nothrow, _validate_capacity(capacity), huge_pages_policy, reader_store_percent)
  {
    if (QUILL_UNLIKELY(!has_storage()))
    {
      int const error = errno;
      QUILL_THROW(QuillError{std::string{"Failed to allocate the queue storage. errno: "} + std::to_string(error)});
    }
  }

  /**
   * Constructs the queue without throwing, e.g. to grow an unbounded queue inside a log call.
   * If the capacity is too large or the allocation fails, has_storage() returns false and the
   * queue must not be used.
   */
  BoundedSPSCQueueImpl(std::nothrow_t, integer_type capacity, HugePagesPolicy huge_pages_policy,
                       integer_type reader_store_percent = 5) noexcept
    : _capacity(static_cast<integer_type>(next_power_of_two(static_cast<size_t>(capacity)))),
      _mask(_capacity - 1),
      _bytes_per_batch(static_cast<integer_type>(
        (static_cast<double>(_capacity) * static_cast<double>(reader_store_percent)) / 100.0)),
      _huge_pages_policy(huge_pages_policy)
  {
    if (!_is_supported_capacity(next_power_of_two(static_cast<size_t>(capacity))))
    {
      return;
    }

    _storage = static_cast<std::byte*>(_alloc_aligned(2u * static_cast<size_t>(_capacity),
                                                      QUILL_CACHE_LINE_ALIGNED, huge_pages_policy));

    if (!_storage)
    {
      return;
    }

    // Initialise and pre-fault the logical queue region. The second half is scratch space for
    // records that cross the ring boundary and is written before it is read.
    std::memset(_storage, 0, static_cast<size_t>(_capacity));

    _atomic_writer_pos.store(0);
    _atomic_reader_pos.store(0);

    // reader_pos starts at 0, so cached value is 0 + capacity
    _reader_pos_cache_plus_capacity = _capacity;
  }

  ~BoundedSPSCQueueImpl() { _free_aligned(_storage); }

  /**
   * Deleted
   */
  BoundedSPSCQueueImpl(BoundedSPSCQueueImpl const&) = delete;
  BoundedSPSCQueueImpl& operator=(BoundedSPSCQueueImpl const&) = delete;

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::byte* prepare_write(integer_type n) noexcept
  {
    if (static_cast<integer_type>(_reader_pos_cache_plus_capacity - _writer_pos) < n)
    {
      // not enough space, we need to load reader and re-check
      _reader_pos_cache_plus_capacity = _atomic_reader_pos.load(std::memory_order_acquire) + _capacity;

      if (static_cast<integer_type>(_reader_pos_cache_plus_capacity - _writer_pos) < n)
      {
        return nullptr;
      }
    }

    // Successful writes always have backing storage, as in prepare_write_reserve_cached().
    QUILL_ASSUME(_storage != nullptr);
    return _storage + (_writer_pos & _mask);
  }

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT WriteReservation prepare_write_reserve_cached(integer_type n) noexcept
  {
    integer_type const writer_pos = _writer_pos;

    if (QUILL_UNLIKELY(static_cast<integer_type>(_reader_pos_cache_plus_capacity - writer_pos) < n))
    {
      return WriteReservation{nullptr, writer_pos, this};
    }

    // _storage is never null after construction; hint lets the compiler
    // eliminate a redundant null-check on the computed write pointer.
    QUILL_ASSUME(_storage != nullptr);

    return WriteReservation{_storage + (writer_pos & _mask), writer_pos, this};
  }

  QUILL_ATTRIBUTE_HOT void finish_write(integer_type n) noexcept { _writer_pos += n; }

  QUILL_ATTRIBUTE_HOT void commit_write() noexcept
  {
    // set the atomic flag so the reader can see write
    _atomic_writer_pos.store(_writer_pos, std::memory_order_release);
  }

  /**
   * Finish and commit write as a single function
   */
  QUILL_ATTRIBUTE_HOT void finish_and_commit_write(integer_type n) noexcept
  {
    finish_and_commit_write_reservation(_writer_pos + n);
  }

  QUILL_ATTRIBUTE_HOT void finish_and_commit_write_reservation(integer_type new_writer_pos) noexcept
  {
    _writer_pos = new_writer_pos;

    // set the atomic flag so the reader can see write
    _atomic_writer_pos.store(new_writer_pos, std::memory_order_release);
  }

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::byte* prepare_read() noexcept
  {
    if (empty())
    {
      return nullptr;
    }

    return _storage + (_reader_pos & _mask);
  }

  QUILL_ATTRIBUTE_HOT void finish_read(integer_type n) noexcept { _reader_pos += n; }

  QUILL_ATTRIBUTE_HOT void commit_read() noexcept
  {
    // Publish the read position when a full batch of _bytes_per_batch was consumed, and also when
    // the queue is drained as far as the reader knows (_writer_pos_cache == _reader_pos). Without
    // the drained check, a residual lag smaller than _bytes_per_batch is never released back to
    // the producer once the queue empties, and a subsequent message that fits in the capacity but
    // not in (capacity - lag) can never be written
    if ((static_cast<integer_type>(_reader_pos - _atomic_reader_pos.load(std::memory_order_relaxed)) >= _bytes_per_batch) ||
        (_writer_pos_cache == _reader_pos))
    {
      _atomic_reader_pos.store(_reader_pos, std::memory_order_release);
    }
  }

  /**
   * Only meant to be called by the reader
   * @return true if the queue is empty
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool empty() const noexcept
  {
    if (_writer_pos_cache == _reader_pos)
    {
      // if we think the queue is empty we also load the atomic variable to check further
      _writer_pos_cache = _atomic_writer_pos.load(std::memory_order_acquire);

      if (_writer_pos_cache == _reader_pos)
      {
        return true;
      }
    }

    return false;
  }

  QUILL_NODISCARD integer_type capacity() const noexcept
  {
    return static_cast<integer_type>(_capacity);
  }

  QUILL_NODISCARD HugePagesPolicy huge_pages_policy() const noexcept { return _huge_pages_policy; }

  /** @return false if a std::nothrow constructed queue could not allocate its storage */
  QUILL_NODISCARD bool has_storage() const noexcept { return _storage != nullptr; }

private:
  /**
   * align a pointer to the given alignment
   * @param pointer a pointer the object
   * @param alignment a pointer the object
   * @return an aligned pointer for the given object
   */
  QUILL_NODISCARD static std::byte* _align_pointer(void* pointer, size_t alignment) noexcept
  {
    QUILL_ASSERT(is_power_of_two(alignment),
                 "alignment must be a power of two in BoundedSPSCQueue::_align_pointer()");
    return reinterpret_cast<std::byte*>((reinterpret_cast<uintptr_t>(pointer) + (alignment - 1ul)) &
                                        ~(alignment - 1ul));
  }

  /** Checks a power-of-two capacity fits the position type and the doubled byte allocation. */
  QUILL_NODISCARD static constexpr bool _is_supported_capacity(size_t rounded_capacity) noexcept
  {
    constexpr size_t max_integer_capacity = []() constexpr
    {
      // avoid including limits
      if constexpr (sizeof(integer_type) >= sizeof(size_t))
      {
        return SIZE_MAX;
      }
      else if constexpr (sizeof(integer_type) <= sizeof(uint8_t))
      {
        return static_cast<size_t>(UINT8_MAX);
      }
      else if constexpr (sizeof(integer_type) <= sizeof(uint16_t))
      {
        return static_cast<size_t>(UINT16_MAX);
      }
      else if constexpr (sizeof(integer_type) <= sizeof(uint32_t))
      {
        return static_cast<size_t>(UINT32_MAX);
      }
      else
      {
        return static_cast<size_t>(UINT64_MAX);
      }
    }();

    // The logical capacity is tracked as integer_type, but the backing allocation is byte-sized:
    // two mirrored queue regions plus POSIX alignment metadata. Validate the allocation size in
    // size_t before multiplying by two, otherwise an extreme capacity can wrap to a tiny mmap.
    constexpr size_t metadata_size{2u * sizeof(size_t)};
    constexpr size_t max_rounded_capacity = (SIZE_MAX - metadata_size - QUILL_CACHE_LINE_ALIGNED) / 2u;

    return (rounded_capacity <= max_integer_capacity) && (rounded_capacity <= max_rounded_capacity);
  }

  /**
   * Validate the requested capacity before any allocation happens.
   * @throws QuillError if the capacity is not supported
   */
  static integer_type _validate_capacity(integer_type capacity)
  {
    size_t const rounded_capacity = next_power_of_two(static_cast<size_t>(capacity));

    if (QUILL_UNLIKELY(!_is_supported_capacity(rounded_capacity)))
    {
      QUILL_THROW(QuillError{"BoundedSPSCQueue capacity is too large"});
    }

    return static_cast<integer_type>(rounded_capacity);
  }

  /**
   * Aligned alloc
   * @param size number of bytes to allocate. An integral multiple of alignment
   * @param alignment specifies the alignment. Must be a valid alignment supported by the implementation.
   * @param huge_pages_policy allocate huge pages, only supported on linux
   * @return On success, returns the pointer to the beginning of newly allocated memory.
   * To avoid a memory leak, the returned pointer must be deallocated with _free_aligned().
   * On failure returns nullptr with errno set.
   */
  QUILL_NODISCARD static void* _alloc_aligned(size_t size, size_t alignment,
                                              QUILL_MAYBE_UNUSED HugePagesPolicy huge_pages_policy) noexcept
  {
#if defined(_WIN32)
    return _aligned_malloc(size, alignment);
#else
    // Calculate the total size including the metadata and alignment
    constexpr size_t metadata_size{2u * sizeof(size_t)};
    size_t total_size{size + metadata_size + alignment};

    // Allocate the memory
    int flags = MAP_PRIVATE | MAP_ANONYMOUS;

  #if defined(__linux__)
    if (huge_pages_policy != HugePagesPolicy::Never)
    {
      // Request 2 MiB pages explicitly so the page size is known. The rounding cannot
      // overflow: _validate_capacity() caps the capacity well below SIZE_MAX / 2
      constexpr size_t huge_page_size{2u * 1024u * 1024u};
      total_size = ((total_size + huge_page_size - 1u) / huge_page_size) * huge_page_size;
      flags |= MAP_HUGETLB | MAP_HUGE_2MB;
    }
  #endif

    void* mem = ::mmap(nullptr, total_size, PROT_READ | PROT_WRITE, flags, -1, 0);

  #if defined(__linux__)
    if ((mem == MAP_FAILED) && (huge_pages_policy == HugePagesPolicy::Try))
    {
      // we tried but failed allocating huge pages, try normal pages instead. Normal pages do
      // not need the huge-page-size rounding of the length
      flags &= ~(MAP_HUGETLB | MAP_HUGE_2MB);
      total_size = size + metadata_size + alignment;
      mem = ::mmap(nullptr, total_size, PROT_READ | PROT_WRITE, flags, -1, 0);
    }
  #endif

    if (mem == MAP_FAILED)
    {
      return nullptr;
    }

    // Calculate the aligned address after the metadata
    std::byte* aligned_address = _align_pointer(static_cast<std::byte*>(mem) + metadata_size, alignment);

    // Calculate the offset from the original memory location
    auto const offset = static_cast<size_t>(aligned_address - static_cast<std::byte*>(mem));

    // Store the size and offset information in the metadata
    std::memcpy(aligned_address - sizeof(size_t), &total_size, sizeof(total_size));
    std::memcpy(aligned_address - (2u * sizeof(size_t)), &offset, sizeof(offset));

    return aligned_address;
#endif
  }

  /**
   * Free aligned memory allocated with _alloc_aligned
   * @param ptr address to aligned memory
   */
  void static _free_aligned(void* ptr) noexcept
  {
    if (!ptr)
    {
      return;
    }

#if defined(_WIN32)
    _aligned_free(ptr);
#else
    // Retrieve the size and offset information from the metadata
    size_t offset;
    std::memcpy(&offset, static_cast<std::byte*>(ptr) - (2u * sizeof(size_t)), sizeof(offset));

    size_t total_size;
    std::memcpy(&total_size, static_cast<std::byte*>(ptr) - sizeof(size_t), sizeof(total_size));

    // Calculate the original memory block address
    void* mem = static_cast<std::byte*>(ptr) - offset;

    ::munmap(mem, total_size);
#endif
  }

private:
  integer_type const _capacity;
  integer_type const _mask;
  integer_type const _bytes_per_batch;
  std::byte* _storage{nullptr};
  HugePagesPolicy const _huge_pages_policy;

  alignas(QUILL_CACHE_LINE_ALIGNED) std::atomic<integer_type> _atomic_writer_pos{0};
  alignas(QUILL_CACHE_LINE_ALIGNED) integer_type _writer_pos{0};
  integer_type _reader_pos_cache_plus_capacity{0};

  alignas(QUILL_CACHE_LINE_ALIGNED) std::atomic<integer_type> _atomic_reader_pos{0};
  alignas(QUILL_CACHE_LINE_ALIGNED) integer_type _reader_pos{0};
  mutable integer_type _writer_pos_cache{0};
};

using BoundedSPSCQueue = BoundedSPSCQueueImpl<size_t>;

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
  #pragma warning(pop)
#endif

} // namespace detail

QUILL_END_NAMESPACE
