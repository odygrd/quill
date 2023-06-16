#pragma once

#include "quill/QuillError.h"
#include "quill/TweakMe.h"
#include "quill/detail/misc/Attributes.h"
#include "quill/detail/misc/Common.h"
#include "quill/detail/misc/Os.h"
#include "quill/detail/misc/Utilities.h"

#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>

#if defined(QUILL_X86ARCH)
  #if defined(_WIN32)
    #include <intrin.h>
  #elif defined(__GNUC__) && __GNUC__ > 10
    #include <emmintrin.h>
    #include <x86gprintrin.h>
  #elif defined(__clang_major__)
    // clang needs immintrin for _mm_clflushopt
    #include <immintrin.h>
  #else
    // older compiler versions do not have <x86gprintrin.h>
    #include <immintrin.h>
    #include <x86intrin.h>
  #endif
#endif

namespace quill::detail
{
/**
 * A bounded single producer single consumer ring buffer.
 */
template <typename T>
class BoundedQueueImpl
{
public:
  using integer_type = T;

  QUILL_ALWAYS_INLINE explicit BoundedQueueImpl(integer_type capacity, bool huge_pages = false)
    : _capacity(next_power_of_2(capacity)),
      _mask(_capacity - 1),
      _storage(static_cast<std::byte*>(
        alloc_aligned(2ull * static_cast<uint64_t>(_capacity), CACHE_LINE_ALIGNED, huge_pages)))
  {
    if (!is_pow_of_two(static_cast<uint64_t>(_capacity)))
    {
      QUILL_THROW(QuillError{"capacity must be a power of two. _capacity: " + std::to_string(_capacity)});
    }

    std::memset(_storage, 0, 2ull * static_cast<uint64_t>(_capacity));

    _atomic_writer_pos.store(0);
    _atomic_reader_pos.store(0);

#if defined(QUILL_X86ARCH)
    // remove log memory from cache
    for (uint64_t i = 0; i < (2ull * static_cast<uint64_t>(_capacity)); i += CACHE_LINE_SIZE)
    {
      _mm_clflush(_storage + i);
    }

    // load cache lines into memory
    if (_capacity < 1024)
    {
      QUILL_THROW(QuillError{"Capacity must be at least 1024"});
    }

    uint64_t const cache_lines = (_capacity >= 2048) ? 32 : 16;

    for (uint64_t i = 0; i < cache_lines; ++i)
    {
      _mm_prefetch(reinterpret_cast<char const*>(_storage + (CACHE_LINE_SIZE * i)), _MM_HINT_T0);
    }
#endif
  }

  ~BoundedQueueImpl() { free_aligned(_storage); }

  /**
   * Deleted
   */
  BoundedQueueImpl(BoundedQueueImpl const&) = delete;
  BoundedQueueImpl& operator=(BoundedQueueImpl const&) = delete;

  QUILL_NODISCARD_ALWAYS_INLINE_HOT std::byte* prepare_write(integer_type n) noexcept
  {
    if ((_capacity - static_cast<integer_type>(_writer_pos - _reader_pos_cache)) >= n)
    {
      return _storage + (_writer_pos & _mask);
    }

    // not enough space, we need to load reader and re-check
    _reader_pos_cache = _atomic_reader_pos.load(std::memory_order_acquire);

    return ((_capacity - static_cast<integer_type>(_writer_pos - _reader_pos_cache)) >= n)
      ? _storage + (_writer_pos & _mask)
      : nullptr;
  }

  QUILL_ALWAYS_INLINE_HOT void finish_write(integer_type n) noexcept { _writer_pos += n; }

  QUILL_ALWAYS_INLINE_HOT void commit_write() noexcept
  {
#if defined(QUILL_X86ARCH)
    // flush writen cache lines
    _flush_cachelines(_last_flushed_writer_pos, _writer_pos);

    // prefetch a future cache line
    _mm_prefetch(reinterpret_cast<char const*>(_storage + ((_writer_pos + (CACHE_LINE_SIZE * 10)) & _mask)),
                 _MM_HINT_T0);
#endif

    // set the atomic flag so the reader can see write
    _atomic_writer_pos.store(_writer_pos, std::memory_order_release);
  }

  QUILL_NODISCARD_ALWAYS_INLINE_HOT std::byte* prepare_read() noexcept
  {
    if (static_cast<integer_type>(_writer_pos_cache - _reader_pos) != 0)
    {
      return _storage + (_reader_pos & _mask);
    }

    // nothing to read, try to load the writer_pos again
    _writer_pos_cache = _atomic_writer_pos.load(std::memory_order_acquire);

    return (static_cast<integer_type>(_writer_pos_cache - _reader_pos) != 0)
      ? _storage + (_reader_pos & _mask)
      : nullptr;
  }

  QUILL_ALWAYS_INLINE_HOT void finish_read(integer_type n) noexcept { _reader_pos += n; }

  QUILL_ALWAYS_INLINE_HOT void commit_read() noexcept
  {
#if defined(QUILL_X86ARCH)
    _flush_cachelines(_last_flushed_reader_pos, _reader_pos);
#endif

    _atomic_reader_pos.store(_reader_pos, std::memory_order_release);
  }

  QUILL_NODISCARD bool empty() const noexcept
  {
    return _atomic_reader_pos.load(std::memory_order_relaxed) ==
      _atomic_writer_pos.load(std::memory_order_relaxed);
  }

  QUILL_NODISCARD integer_type capacity() const noexcept
  {
    return static_cast<integer_type>(_capacity);
  }

private:
#if defined(QUILL_X86ARCH)
  QUILL_ALWAYS_INLINE_HOT void _flush_cachelines(integer_type& last, integer_type offset)
  {
    integer_type last_diff = last - (last & CACHELINE_MASK);
    integer_type const cur_diff = offset - (offset & CACHELINE_MASK);

    while (cur_diff > last_diff)
    {
      _mm_clflushopt(_storage + (last_diff & _mask));
      last_diff += CACHE_LINE_SIZE;
      last = last_diff;
    }
  }
#endif

private:
  static constexpr integer_type CACHELINE_MASK{CACHE_LINE_SIZE - 1};

  integer_type const _capacity;
  integer_type const _mask;
  std::byte* const _storage{nullptr};

  alignas(CACHE_LINE_ALIGNED) std::atomic<integer_type> _atomic_writer_pos{0};
  integer_type _writer_pos{0};
  integer_type _last_flushed_writer_pos{0};
  integer_type _reader_pos_cache{0};

  alignas(CACHE_LINE_ALIGNED) std::atomic<integer_type> _atomic_reader_pos{0};
  integer_type _reader_pos{0};
  integer_type _last_flushed_reader_pos{0};
  integer_type _writer_pos_cache{0};
};

using BoundedQueue = BoundedQueueImpl<uint32_t>;
} // namespace quill::detail
