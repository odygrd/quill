#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>
#include <stdexcept>

#if defined(QUILL_X86ARC)
  #include <emmintrin.h>
  #include <immintrin.h>
  #include <x86intrin.h>
#endif

#include "quill/QuillError.h"
#include "quill/detail/misc/Attributes.h"
#include "quill/detail/misc/Common.h"
#include "quill/detail/misc/Os.h"
#include "quill/detail/misc/Utilities.h"

namespace quill::detail
{
class BoundedQueue
{
public:
  BoundedQueue(int32_t capacity)
    : _capacity(capacity),
      _mask(_capacity - 1),
      _storage(static_cast<std::byte*>(aligned_alloc(CACHELINE_SIZE, static_cast<size_t>(2ll * capacity))))
  {
    if (!is_pow_of_two(static_cast<size_t>(capacity)))
    {
      QUILL_THROW(std::runtime_error{"Capacity must be a power of two"});
    }

    std::memset(_storage, 0, static_cast<size_t>(2ll * capacity));

#if defined(QUILL_X86ARC)
    // eject log memory from cache
    for (uint32_t i = 0; i < (2ul * capacity); i += CACHELINE_SIZE)
    {
      _mm_clflush(_storage + i);
    }

    // load first 16 cache lines into memory
    for (int i = 0; i < 16; ++i)
    {
      _mm_prefetch(_storage + (i * CACHELINE_SIZE), _MM_HINT_T0);
    }
#endif
  }

  ~BoundedQueue() { aligned_free(_storage); }

  QUILL_NODISCARD_ALWAYS_INLINE_HOT int32_t get_writer_pos() const noexcept
  {
    return _writer_pos & _mask;
  }
  QUILL_NODISCARD_ALWAYS_INLINE_HOT int32_t get_writer_pos(int32_t diff) const noexcept
  {
    return (_writer_pos + diff) & _mask;
  }

  QUILL_NODISCARD_ALWAYS_INLINE_HOT int32_t get_reader_pos() const noexcept
  {
    return _reader_pos & _mask;
  }

  QUILL_NODISCARD_ALWAYS_INLINE_HOT std::byte* prepare_write(int32_t n) noexcept
  {
    // check occupied (writer - reader) > _capacity - n_requested
    if ((_writer_pos - _reader_pos_cache) > (_capacity - n))
    {
      // not enough space, we need to load reader and re-check
      _reader_pos_cache = _atomic_reader_pos.load(std::memory_order_acquire);
      return ((_writer_pos - _reader_pos_cache) > (_capacity - n)) ? nullptr : _storage + get_writer_pos();
    }
    return _storage + get_writer_pos();
  }

  QUILL_ALWAYS_INLINE_HOT void commit_write(int32_t n) noexcept
  {
    _writer_pos += n;

#if defined(QUILL_X86ARC)
    // flush writen cache lines
    _flush_cachelines(_last_flushed_writer_pos, _writer_pos);

    // prefetch a future cacheline
    _mm_prefetch(_storage + get_writer_pos(CACHELINE_SIZE * 8), _MM_HINT_T0);
#endif

    // set the atomic flag so the reader can see write
    _atomic_writer_pos.store(_writer_pos, std::memory_order_release);
  }

  QUILL_NODISCARD_ALWAYS_INLINE_HOT std::byte* prepare_read() noexcept
  {
    if ((_writer_pos_cache - _reader_pos) < 1)
    {
      // nothing to read, try to load the writer_pos again
      _writer_pos_cache = _atomic_writer_pos.load(std::memory_order_acquire);
      return (_writer_pos_cache - _reader_pos) < 1 ? nullptr : _storage + get_reader_pos();
    }
    return _storage + get_reader_pos();
  }

  QUILL_ALWAYS_INLINE_HOT void finish_read(int32_t n) noexcept
  {
    _reader_pos += n;

#if defined(QUILL_X86ARC)
    _flush_cachelines(_last_flushed_reader_pos, _reader_pos);
#endif

    _atomic_reader_pos.store(_reader_pos, std::memory_order_release);
  }

  QUILL_NODISCARD bool empty() const noexcept
  {
    return _atomic_reader_pos.load(std::memory_order_relaxed) ==
      _atomic_writer_pos.load(std::memory_order_relaxed);
  }

  QUILL_NODISCARD int32_t capacity() const noexcept { return _capacity; }

#if defined(QUILL_X86ARC)
private:
  QUILL_ALWAYS_INLINE_HOT void _flush_cachelines(int32_t& last, int32_t offset)
  {
    int32_t last_diff = last - (last & CACHELINE_MASK);
    int32_t const cur_diff = offset - (offset & CACHELINE_MASK);

    while (cur_diff > last_diff)
    {
      _mm_clflushopt(_storage + (last_diff & _mask));
      last_diff += CACHELINE_SIZE;
      last = last_diff;
    }
  }
#endif

private:
  static constexpr int32_t CACHELINE_MASK{CACHELINE_SIZE - 1};

  int32_t const _capacity;
  int32_t const _mask;
  std::byte* const _storage{nullptr};

  alignas(CACHELINE_SIZE) std::atomic<int32_t> _atomic_writer_pos{0};
  int32_t _writer_pos{0};
  int32_t _last_flushed_writer_pos{0};
  int32_t _reader_pos_cache{0};

  alignas(CACHELINE_SIZE) std::atomic<int32_t> _atomic_reader_pos{0};
  int32_t _reader_pos{0};
  int32_t _last_flushed_reader_pos{0};
  int32_t _writer_pos_cache{0};
};
} // namespace quill::detail