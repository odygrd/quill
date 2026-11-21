/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"

#include "quill/bundled/fmt/base.h"

#ifndef QUILL_MODULE
  #include <cstddef>
  #include <cstdint>
  #include <cstring>
  #include <string_view>
#endif

QUILL_BEGIN_NAMESPACE

namespace detail
{
/** Appends text that does not fit the buffer's current capacity */
QUILL_NOINLINE inline void append_to_buffer_and_grow(fmtquill::detail::buffer<char>& buffer, std::string_view text)
{
  // fmt's append() grows the buffer, or flushes a buffer with a fixed capacity, as needed
  buffer.append(text.data(), text.data() + text.size());
}

/**
 * Appends `text` to `buffer`.
 *
 * The backend builds each log statement from many short pieces, such as pattern literals, the
 * log level and the thread id, and a call to memcpy costs more than copying a few bytes. When the
 * text fits the buffer's capacity, up to 16 bytes are copied with two overlapping fixed-size moves,
 * which read and write only the bytes of `text`, and longer text with one memcpy. Text that needs
 * the buffer to grow is appended out of line, so this function stays small enough to inline.
 */
QUILL_ATTRIBUTE_HOT inline void append_to_buffer(fmtquill::detail::buffer<char>& buffer, std::string_view text)
{
  size_t const size = text.size();
  size_t const offset = buffer.size();
  size_t const new_size = offset + size;

  if (QUILL_UNLIKELY(new_size > buffer.capacity()))
  {
    append_to_buffer_and_grow(buffer, text);
    return;
  }

  // The text fits the capacity, so this only sets the size. The check above is the same expression
  // that try_resize() tests before growing, which lets the compiler drop its grow call.
  buffer.try_resize(new_size);

  char* const dest = buffer.data() + offset;
  char const* const src = text.data();

  if (size > 16)
  {
    std::memcpy(dest, src, size);
  }
  else if (size >= 8)
  {
    uint64_t head;
    uint64_t tail;
    std::memcpy(&head, src, sizeof(head));
    std::memcpy(&tail, src + size - sizeof(tail), sizeof(tail));
    std::memcpy(dest, &head, sizeof(head));
    std::memcpy(dest + size - sizeof(tail), &tail, sizeof(tail));
  }
  else if (size >= 4)
  {
    uint32_t head;
    uint32_t tail;
    std::memcpy(&head, src, sizeof(head));
    std::memcpy(&tail, src + size - sizeof(tail), sizeof(tail));
    std::memcpy(dest, &head, sizeof(head));
    std::memcpy(dest + size - sizeof(tail), &tail, sizeof(tail));
  }
  else if (size != 0)
  {
    dest[0] = src[0];
    dest[size / 2] = src[size / 2];
    dest[size - 1] = src[size - 1];
  }
}
} // namespace detail

QUILL_END_NAMESPACE
