/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Common.h"

#include "quill/bundled/fmt/base.h"

#ifndef QUILL_MODULE
  #include <cstddef>
  #include <cstring>
  #include <memory>
  #include <string_view>
  #include <utility>
  #include <vector>
#endif

QUILL_BEGIN_NAMESPACE

namespace detail
{
/**
 * Stores the formatted messages of buffered events, first in, first out.
 *
 * The backend formats the message of an event when it reads the event from the queue of a
 * thread, and reads the message again when it writes the event to the sinks, in the same order.
 * The arena keeps the messages back to back in that order, in blocks of memory that it reuses,
 * so both passes go through memory sequentially whatever the length of a message. A buffer per
 * event is wherever the allocator placed it, and with thousands of events buffered reading each
 * one is a cache miss.
 *
 * A message is formatted in place: begin_message() starts one after the newest message and
 * returns the fmt buffer to write it to, and commit_message() keeps it. pop_front() releases the
 * oldest message. A committed message does not move until it is released.
 *
 * The blocks form a ring. Messages fill one block after the other, and a block is written from
 * its start again once all its messages have been released, so while the backend keeps up with
 * the frontend the same memory is used again and again. A block that was allocated for a message
 * longer than a block is the exception: it is freed once its messages have been released.
 */
class MessageArena : private fmtquill::detail::buffer<char>
{
public:
  /**
   * @param block_capacity the size of a block in bytes. A longer message gets a block that fits it
   */
  explicit MessageArena(size_t block_capacity)
    : fmtquill::detail::buffer<char>(_grow), _block_capacity(block_capacity)
  {
    // A block without memory, so that there is always a back block: the first message that is
    // written allocates its memory
    _blocks.emplace_back();
  }

  MessageArena(MessageArena const&) = delete;
  MessageArena& operator=(MessageArena const&) = delete;

  /**
   * Moves the committed messages, which stay where they are. A message that was begun is not
   * moved, and the moved-from arena can only be destroyed or assigned to.
   */
  MessageArena(MessageArena&& other) noexcept
    : fmtquill::detail::buffer<char>(_grow),
      _blocks(std::move(other._blocks)),
      _front(other._front),
      _back(other._back),
      _block_capacity(other._block_capacity)
  {
  }

  MessageArena& operator=(MessageArena&& other) noexcept
  {
    if (this != &other)
    {
      _blocks = std::move(other._blocks);
      _front = other._front;
      _back = other._back;
      _block_capacity = other._block_capacity;
      _set_message(nullptr, 0);
    }

    return *this;
  }

  /**
   * Starts a message after the newest one. A message that was begun and not committed is dropped.
   * @return the buffer to write the message to, which is empty. Writing to it throws
   * std::bad_alloc when a block cannot be allocated, and the committed messages are then unchanged
   */
  QUILL_ATTRIBUTE_HOT fmtquill::detail::buffer<char>& begin_message() noexcept
  {
    Block const& block = _blocks[_back];
    _set_message(block.data.get() + block.size, block.capacity - block.size);
    return *this;
  }

  /** @return the buffer of the message that begin_message() started */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT fmtquill::detail::buffer<char>& message() noexcept
  {
    return *this;
  }

  /**
   * Keeps what was written to the buffer since begin_message() as the newest message. It can be
   * empty.
   * @return the message, valid until pop_front() releases it
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::string_view commit_message() noexcept
  {
    std::string_view const committed{this->data(), this->size()};

    Block& block = _blocks[_back];
    block.size += committed.size();
    ++block.message_count;

    // The buffer continues after the message, so nothing can be appended to it
    _set_message(this->data() + committed.size(), this->capacity() - committed.size());

    return committed;
  }

  /** Releases the oldest message. The arena must not be empty */
  QUILL_ATTRIBUTE_HOT void pop_front() noexcept
  {
    Block& block = _blocks[_front];

    QUILL_ASSERT(block.message_count != 0, "MessageArena::pop_front() called on an empty arena");

    --block.message_count;

    if (block.message_count == 0)
    {
      // Every message of the block has been released, so it is written from its start again
      block.size = 0;

      if (QUILL_UNLIKELY(block.capacity > _block_capacity))
      {
        // The block was allocated for a message longer than a block. Reused, it would keep that
        // memory for as long as the arena lives, e.g. a megabyte per message that a backtrace
        // stored once, so it is freed and allocated with the size of a block when written again.
        block.data.reset();
        block.capacity = 0;

        if (_front == _back)
        {
          // The buffer pointed into the freed memory
          _set_message(nullptr, 0);
        }
      }

      if (_front != _back)
      {
        _front = _next_index(_front);
      }
    }
  }

  /** Releases every message and frees the memory of the blocks */
  void reset() noexcept
  {
    _blocks.erase(_blocks.begin() + 1, _blocks.end());
    _blocks.front() = Block{};
    _front = 0;
    _back = 0;
    _set_message(nullptr, 0);
  }

  /** @return true when no message is committed */
  QUILL_NODISCARD bool empty() const noexcept { return _blocks[_front].message_count == 0; }

  /** @return the bytes of memory that the blocks have allocated */
  QUILL_NODISCARD size_t allocated_bytes() const noexcept
  {
    size_t bytes{0};

    for (Block const& block : _blocks)
    {
      bytes += block.capacity;
    }

    return bytes;
  }

private:
  struct Block
  {
    std::unique_ptr<char[]> data;
    size_t capacity{0};
    size_t size{0};          /**< bytes of committed messages, from the start of the block */
    size_t message_count{0}; /**< committed messages of the block that have not been released */
  };

  QUILL_NODISCARD size_t _next_index(size_t index) const noexcept
  {
    return ((index + 1) == _blocks.size()) ? 0 : (index + 1);
  }

  /** Points the fmt buffer at the place of the message that is written next, with nothing written */
  void _set_message(char* data, size_t capacity) noexcept
  {
    this->set(data, capacity);
    this->clear();
  }

  /** Copies the `written` bytes of the message that is being written to the start of a block */
  void _copy_message(char* block_data, size_t written) noexcept
  {
    if ((written != 0) && (block_data != this->data()))
    {
      std::memcpy(block_data, this->data(), written);
    }
  }

  /** fmt calls this when the message that is being written needs `capacity` bytes */
  static void _grow(fmtquill::detail::buffer<char>& buffer, size_t capacity)
  {
    static_cast<MessageArena&>(buffer)._move_message(capacity);
  }

  /**
   * Moves the message that is being written to a block with room for `capacity` bytes: the block
   * after the back block, or a larger back block when that one holds no committed message.
   */
  QUILL_ATTRIBUTE_COLD void _move_message(size_t capacity)
  {
    size_t const written = this->size();

    // A long message grows by half its size, so that it is not copied for every piece appended
    size_t new_capacity = written + (written / 2);
    new_capacity = (new_capacity > capacity) ? new_capacity : capacity;
    new_capacity = (new_capacity > _block_capacity) ? new_capacity : _block_capacity;

    size_t index = _back;

    if (_blocks[_back].message_count != 0)
    {
      index = _next_index(_back);

      if (index == _front)
      {
        // Every block holds committed messages, so add one after the back block
        index = _back + 1;
        _blocks.emplace(_blocks.begin() + static_cast<std::ptrdiff_t>(index));

        if (_front >= index)
        {
          ++_front;
        }
      }
    }

    Block& block = _blocks[index];

    if (block.capacity < new_capacity)
    {
      // Not value-initialized, as the messages overwrite it. The message is copied before the
      // block frees its memory, which is where the message is when it stays in the back block
      std::unique_ptr<char[]> data{new char[new_capacity]};
      _copy_message(data.get(), written);
      block.data = std::move(data);
      block.capacity = new_capacity;
    }
    else
    {
      _copy_message(block.data.get(), written);
    }

    _back = index;
    this->set(block.data.get(), block.capacity);
  }

  /**
   * In ring order from _front to _back each block holds at least one committed message, except
   * _back while it holds only the message that is being written. The other blocks hold none and
   * are the next ones to be written.
   */
  std::vector<Block> _blocks;
  size_t _front{0}; /**< the block of the oldest message */
  size_t _back{0};  /**< the block of the newest message, which the next one is written to */
  size_t _block_capacity;
};

} // namespace detail

QUILL_END_NAMESPACE
