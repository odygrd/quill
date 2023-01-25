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
  #include <emmintrin.h>
  #include <immintrin.h>
  #include <x86intrin.h>
#endif

namespace quill::detail
{
/**
 * A bounded single producer single consumer ring buffer.
 */
class BoundedQueue
{
public:
  QUILL_ALWAYS_INLINE explicit BoundedQueue(uint32_t capacity)
    : _capacity(capacity),
      _mask(_capacity - 1),
      _storage(static_cast<std::byte*>(aligned_alloc(CACHE_LINE_ALIGNED, 2ull * capacity)))
  {
    if (!is_pow_of_two(static_cast<size_t>(capacity)))
    {
      QUILL_THROW(std::runtime_error{"Capacity must be a power of two"});
    }

    std::memset(_storage, 0, 2ull * capacity);

#if defined(QUILL_X86ARCH)
    // remove log memory from cache
    for (uint64_t i = 0; i < (2ull * capacity); i += CACHE_LINE_SIZE)
    {
      _mm_clflush(_storage + i);
    }

    // load cache lines into memory
    if (capacity < 1024)
    {
      QUILL_THROW(std::runtime_error{"Capacity must be at least 1024"});
    }

    uint64_t const cache_lines = (capacity >= 2048) ? 32 : 16;

    for (uint64_t i = 0; i < cache_lines; ++i)
    {
      _mm_prefetch(_storage + (CACHE_LINE_SIZE * i), _MM_HINT_T0);
    }
#endif
  }

  ~BoundedQueue() { aligned_free(_storage); }

  /**
   * Deleted
   */
  BoundedQueue(BoundedQueue const&) = delete;
  BoundedQueue& operator=(BoundedQueue const&) = delete;

  QUILL_NODISCARD_ALWAYS_INLINE_HOT std::byte* prepare_write(uint32_t n) noexcept
  {
    if ((_capacity - (_writer_pos - _reader_pos_cache)) >= n)
    {
      return _storage + (_writer_pos & _mask);
    }

    // not enough space, we need to load reader and re-check
    _reader_pos_cache = _atomic_reader_pos.load(std::memory_order_acquire);
    return ((_capacity - (_writer_pos - _reader_pos_cache)) >= n) ? _storage + (_writer_pos & _mask) : nullptr;
  }

  QUILL_ALWAYS_INLINE_HOT void finish_write(uint32_t n) noexcept { _writer_pos += n; }

  QUILL_ALWAYS_INLINE_HOT void commit_write() noexcept
  {
#if defined(QUILL_X86ARCH)
    // flush writen cache lines
    _flush_cachelines(_last_flushed_writer_pos, _writer_pos);

    // prefetch a future cache line
    _mm_prefetch(_storage + ((_writer_pos + (CACHE_LINE_SIZE * 10)) & _mask), _MM_HINT_T0);
#endif

    // set the atomic flag so the reader can see write
    _atomic_writer_pos.store(_writer_pos, std::memory_order_release);
  }

  QUILL_NODISCARD_ALWAYS_INLINE_HOT std::byte* prepare_read() noexcept
  {
    if (_writer_pos_cache > _reader_pos)
    {
      return _storage + (_reader_pos & _mask);
    }

    // nothing to read, try to load the writer_pos again
    _writer_pos_cache = _atomic_writer_pos.load(std::memory_order_acquire);
    return (_writer_pos_cache > _reader_pos) ? _storage + (_reader_pos & _mask) : nullptr;
  }

  QUILL_ALWAYS_INLINE_HOT void finish_read(uint32_t n) noexcept { _reader_pos += n; }

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

  QUILL_NODISCARD uint32_t capacity() const noexcept { return static_cast<uint32_t> (_capacity); }

private:
#if defined(QUILL_X86ARCH)
  QUILL_ALWAYS_INLINE_HOT void _flush_cachelines(uint64_t& last, uint64_t offset)
  {
    uint64_t last_diff = last - (last & CACHELINE_MASK);
    uint64_t const cur_diff = offset - (offset & CACHELINE_MASK);

    while (cur_diff > last_diff)
    {
      _mm_clflushopt(_storage + (last_diff & _mask));
      last_diff += CACHE_LINE_SIZE;
      last = last_diff;
    }
  }
#endif

private:
  static constexpr uint64_t CACHELINE_MASK{CACHE_LINE_SIZE - 1};

  uint64_t const _capacity;
  uint64_t const _mask;
  std::byte* const _storage{nullptr};

  alignas(CACHE_LINE_ALIGNED) std::atomic<uint64_t> _atomic_writer_pos{0};
  uint64_t _writer_pos{0};
  uint64_t _last_flushed_writer_pos{0};
  uint64_t _reader_pos_cache{0};

  alignas(CACHE_LINE_ALIGNED) std::atomic<uint64_t> _atomic_reader_pos{0};
  uint64_t _reader_pos{0};
  uint64_t _last_flushed_reader_pos{0};
  uint64_t _writer_pos_cache{0};
};
} // namespace quill::detail
