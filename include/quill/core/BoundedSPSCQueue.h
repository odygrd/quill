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
#endif

#if defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
  #include <intrin.h>
#endif

QUILL_BEGIN_NAMESPACE

namespace detail
{

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
  #pragma warning(push)
  #pragma warning(disable : 4324)
#endif

/**
 * Every queue record starts at a multiple of 8 bytes, so the 8-byte words of a record header never
 * cross a cache line. The producer and the consumer both round each record size up with this.
 * It is not a member of BoundedSPSCQueueImpl, where each queue type would instantiate it
 */
QUILL_NODISCARD constexpr size_t align_queue_record_size(size_t n) noexcept
{
  return (n + 7u) & ~static_cast<size_t>(7u);
}

/** Records larger than this request their cache lines before they are encoded, see
 * prefetch_record_for_write() */
inline constexpr size_t prefetched_record_min_size{256};

/**
 * Prefetches a cache line with write intent. GCC and Clang emit a read prefetch for
 * __builtin_prefetch(ptr, 1) unless the target enables PRFCHW, so x86 uses the instruction
 * directly on x86-64, where CPUs without PREFETCHW execute it as a NOP
 */
QUILL_ATTRIBUTE_HOT inline void prefetch_for_write(QUILL_MAYBE_UNUSED void const* ptr) noexcept
{
#if defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
  _m_prefetchw(const_cast<void*>(ptr));
#elif (defined(__GNUC__) || defined(__clang__)) && defined(__x86_64__)
  __asm__ __volatile__("prefetchw %0" : : "m"(*static_cast<char const*>(ptr)));
#elif defined(__GNUC__) || defined(__clang__)
  // 32-bit x86 CPUs without PREFETCHW may not treat it as a NOP, the builtin follows the target
  __builtin_prefetch(ptr, 1, 3);
#endif
}

/**
 * Requests write ownership of the cache lines of a reserved queue record before it is encoded.
 *
 * The consumer has read every line of the queue, so each line that the producer writes again has
 * to be fetched from the consumer's core first. The L2 hardware prefetcher usually does this ahead
 * of the writes, but it stops doing it for a queue whose pages it has recently seen, e.g. a 128 KiB
 * queue of 32 pages on Golden Cove. The stores of a large record then wait for their lines one
 * after the other and fill the store buffer. Requesting the lines first fetches them in parallel.
 * Only the first 32 lines are requested. Not inlined: the loop would make every call site of a
 * signature with a string or a container larger, and the call costs little next to such a record
 */
QUILL_NOINLINE inline void prefetch_record_for_write(std::byte* record, size_t size) noexcept
{
  constexpr uintptr_t cache_line{64};
  constexpr size_t max_prefetched_bytes{32u * cache_line};

  // The first line is written as soon as encoding starts, a request for it would not be ahead
  uintptr_t line = (reinterpret_cast<uintptr_t>(record) & ~(cache_line - 1u)) + cache_line;
  uintptr_t const end =
    reinterpret_cast<uintptr_t>(record) + ((size < max_prefetched_bytes) ? size : max_prefetched_bytes);

  for (; line < end; line += cache_line)
  {
    prefetch_for_write(reinterpret_cast<void const*>(line));
  }
}

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

    // Pre-fault the logical queue region. The queue never reads bytes it has not written, so this
    // only moves page faults out of the logging path. The second half is scratch space for
    // records that cross the ring boundary. The storage is page aligned, so that half starts on
    // a page of its own: its start is pre-faulted as well, or the first record that crosses the
    // boundary would fault that page in.
    constexpr size_t scratch_prefault_size{4096};
    size_t const logical_size = static_cast<size_t>(_capacity);
    size_t const scratch_size = (logical_size < scratch_prefault_size) ? logical_size : scratch_prefault_size;
    std::memset(_storage, 0, logical_size + scratch_size);
  }

  /**
   * Constructs the queue without throwing, to grow an unbounded queue inside a log call.
   * If the capacity is too large or the allocation fails, has_storage() returns false and the
   * queue must not be used. Unlike the throwing constructor it does not pre-fault the memory,
   * which would stall that log call for a time proportional to the capacity.
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

    _storage = _allocate_storage(2u * static_cast<size_t>(_capacity), huge_pages_policy, _mapped_size);

    if (!_storage)
    {
      return;
    }

    // reader_pos starts at 0, so cached value is 0 + capacity
    _reader_pos_cache_plus_capacity = _capacity;
  }

  ~BoundedSPSCQueueImpl() { _free_storage(_storage, _mapped_size); }

  /**
   * Deleted
   */
  BoundedSPSCQueueImpl(BoundedSPSCQueueImpl const&) = delete;
  BoundedSPSCQueueImpl& operator=(BoundedSPSCQueueImpl const&) = delete;

  /**
   * The free space is a multiple of the record alignment, as the positions and the capacity are,
   * so a record that fits also fits once it is rounded up. The request is compared as given:
   * rounding it up first would wrap a size close to SIZE_MAX to 0
   */
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

    // A queue that is written to has storage, see has_storage(); the hint lets the compiler
    // eliminate a redundant null-check on the computed write pointer.
    QUILL_ASSUME(_storage != nullptr);

    return WriteReservation{_storage + (writer_pos & _mask), writer_pos, this};
  }

  QUILL_ATTRIBUTE_HOT void finish_write(integer_type n) noexcept
  {
    _writer_pos += static_cast<integer_type>(align_queue_record_size(n));
  }

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
    finish_and_commit_write_reservation(_writer_pos + static_cast<integer_type>(align_queue_record_size(n)));
  }

  QUILL_ATTRIBUTE_HOT void finish_and_commit_write_reservation(integer_type new_writer_pos) noexcept
  {
    QUILL_ASSERT(
      new_writer_pos == static_cast<integer_type>(align_queue_record_size(new_writer_pos)),
      "finish_and_commit_write_reservation() requires a position rounded up to the record "
      "alignment, see align_queue_record_size()");

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

  QUILL_ATTRIBUTE_HOT void finish_read(integer_type n) noexcept
  {
    _reader_pos += static_cast<integer_type>(align_queue_record_size(n));
  }

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

  /**
   * Only meant to be called by the reader
   * @return how many bytes were read from the queue, a count that wraps around
   */
  QUILL_NODISCARD integer_type bytes_read() const noexcept { return _reader_pos; }

  /**
   * Only meant to be called by the reader
   * @return how many bytes were written to the queue, a count that wraps around
   */
  QUILL_NODISCARD integer_type bytes_written() const noexcept
  {
    return _atomic_writer_pos.load(std::memory_order_acquire);
  }

  QUILL_NODISCARD integer_type capacity() const noexcept
  {
    return static_cast<integer_type>(_capacity);
  }

  QUILL_NODISCARD HugePagesPolicy huge_pages_policy() const noexcept { return _huge_pages_policy; }

  /** @return false if a std::nothrow constructed queue could not allocate its storage */
  QUILL_NODISCARD bool has_storage() const noexcept { return _storage != nullptr; }

private:
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
    // two mirrored queue regions, rounded up to whole huge pages when requested. Validate in size_t
    // before multiplying by two, otherwise an extreme capacity can wrap to a tiny mmap.
    constexpr size_t huge_page_size{2u * 1024u * 1024u};
    constexpr size_t max_rounded_capacity = (SIZE_MAX - huge_page_size) / 2u;

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
   * Allocates the queue storage without throwing. mmap returns page-aligned memory, which
   * satisfies QUILL_CACHE_LINE_ALIGNED, so the mapping is used directly at its exact size.
   * @param size the bytes of both mirrored halves of the queue
   * @param mapped_size set to the length to release with _free_storage()
   * @return the storage, or nullptr with errno set on failure
   */
  QUILL_NODISCARD static std::byte* _allocate_storage(size_t size, QUILL_MAYBE_UNUSED HugePagesPolicy huge_pages_policy,
                                                      size_t& mapped_size) noexcept
  {
    mapped_size = size;

#if defined(_WIN32)
    return static_cast<std::byte*>(_aligned_malloc(size, QUILL_CACHE_LINE_ALIGNED));
#else
    int const flags = MAP_PRIVATE | MAP_ANONYMOUS;

  #if defined(__linux__)
    if (huge_pages_policy != HugePagesPolicy::Never)
    {
      // Request 2 MiB pages explicitly so the page size is known. MAP_HUGE_2MB can be missing
      // from older libc headers, so build it from the kernel ABI (log2 of the page size << 26).
      constexpr size_t huge_page_size{2u * 1024u * 1024u};
      constexpr int map_huge_2mb{21 << 26};
      size_t const huge_pages_size = ((size + huge_page_size - 1u) / huge_page_size) * huge_page_size;

      void* const mem = ::mmap(nullptr, huge_pages_size, PROT_READ | PROT_WRITE,
                               flags | MAP_HUGETLB | map_huge_2mb, -1, 0);

      if (mem != MAP_FAILED)
      {
        mapped_size = huge_pages_size;
        return static_cast<std::byte*>(mem);
      }

      if (huge_pages_policy == HugePagesPolicy::Always)
      {
        return nullptr;
      }

      // HugePagesPolicy::Try falls back to normal pages
    }
  #endif

    void* const mem = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, flags, -1, 0);
    return (mem == MAP_FAILED) ? nullptr : static_cast<std::byte*>(mem);
#endif
  }

  /** Releases storage from _allocate_storage() */
  static void _free_storage(std::byte* storage, QUILL_MAYBE_UNUSED size_t mapped_size) noexcept
  {
    if (!storage)
    {
      return;
    }

#if defined(_WIN32)
    _aligned_free(storage);
#else
    ::munmap(storage, mapped_size);
#endif
  }

private:
  integer_type const _capacity;
  integer_type const _mask;
  integer_type const _bytes_per_batch;
  std::byte* _storage{nullptr};
  size_t _mapped_size{0}; /**< Length to munmap(); only needed when the queue is destroyed */
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
