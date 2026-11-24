/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/backend/MessageArena.h"
#include "quill/backend/TransitEvent.h"
#include "quill/bundled/fmt/format.h" // for assert_fail
#include "quill/core/Attributes.h"
#include "quill/core/MathUtilities.h"

#ifndef QUILL_MODULE
  #include <cstddef>
  #include <utility>
  #include <vector>
#endif

QUILL_BEGIN_NAMESPACE

namespace detail
{

class TransitEventBuffer
{
public:
  /** The formatted messages are stored in blocks of this many bytes */
  static constexpr size_t message_block_capacity{32u * 1024u};

  explicit TransitEventBuffer(size_t initial_capacity)
    : _initial_capacity(next_power_of_two(initial_capacity)), _mask(_initial_capacity - 1u), _storage(_initial_capacity)
  {
  }

  TransitEventBuffer(TransitEventBuffer const&) = delete;
  TransitEventBuffer& operator=(TransitEventBuffer const&) = delete;

  // Move constructor
  TransitEventBuffer(TransitEventBuffer&& other) noexcept
    : _initial_capacity(other._initial_capacity),
      _mask(other._mask),
      _reader_pos(other._reader_pos),
      _writer_pos(other._writer_pos),
      _shrink_requested(other._shrink_requested),
      _storage(std::move(other._storage)),
      _message_arena(std::move(other._message_arena)),
      _popped_events_before_reset(other._popped_events_before_reset)
  {
    other._mask = 0;
    other._reader_pos = 0;
    other._writer_pos = 0;
    other._shrink_requested = false;
  }

  // Move assignment operator
  TransitEventBuffer& operator=(TransitEventBuffer&& other) noexcept
  {
    if (this != &other)
    {
      _initial_capacity = other._initial_capacity;
      _storage = std::move(other._storage);
      _message_arena = std::move(other._message_arena);
      _mask = other._mask;
      _reader_pos = other._reader_pos;
      _writer_pos = other._writer_pos;
      _popped_events_before_reset = other._popped_events_before_reset;
      _shrink_requested = other._shrink_requested;

      other._mask = 0;
      other._reader_pos = 0;
      other._writer_pos = 0;
      other._shrink_requested = false;
    }
    return *this;
  }

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT TransitEvent* front() noexcept
  {
    if (_reader_pos == _writer_pos)
    {
      return nullptr;
    }
    return &_storage[_reader_pos & _mask];
  }

  /** @return the event that follows front(), or nullptr when there is none */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT TransitEvent const* after_front() const noexcept
  {
    if (size() < 2)
    {
      return nullptr;
    }
    return &_storage[(_reader_pos + 1) & _mask];
  }

  /** Removes front() and releases its formatted message */
  QUILL_ATTRIBUTE_HOT void pop_front() noexcept
  {
    _message_arena.pop_front();
    ++_reader_pos;
  }

  /**
   * Expands the buffer when full, so it can throw on allocation failure
   * @return the event that push_back() adds. Its formatted message is written to back_message()
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT TransitEvent* back()
  {
    if (capacity() == size())
    {
      // Buffer is full, need to expand
      _expand();
    }

    // The event starts without a message, also when the previous one was written and not added
    _message_arena.begin_message();

    return &_storage[_writer_pos & _mask];
  }

  /**
   * @return the buffer that the formatted message of back() is written to. It is empty after
   * back(), and the message is stored after the messages of the events that are already buffered
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT fmtquill::detail::buffer<char>& back_message() noexcept
  {
    return _message_arena.message();
  }

  /** Adds back() to the buffer, with the formatted message that was written to back_message() */
  QUILL_ATTRIBUTE_HOT void push_back() noexcept
  {
    _storage[_writer_pos & _mask].formatted_msg = _message_arena.commit_message();
    ++_writer_pos;
  }

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT size_t size() const noexcept
  {
    return _writer_pos - _reader_pos;
  }

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT size_t capacity() const noexcept { return _storage.size(); }

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool empty() const noexcept
  {
    return _reader_pos == _writer_pos;
  }

  /** @return how many events pop_front() removed, a count that wraps around */
  QUILL_NODISCARD size_t popped_events() const noexcept
  {
    return _popped_events_before_reset + _reader_pos;
  }

  void request_shrink() noexcept { _shrink_requested = true; }

  void try_shrink()
  {
    // we only shrink empty buffers
    if (_shrink_requested && empty())
    {
      if (capacity() > _initial_capacity)
      {
        _storage = std::vector<TransitEvent>(_initial_capacity);
        _mask = _initial_capacity - 1;
        _popped_events_before_reset += _reader_pos;
        _writer_pos = 0;
        _reader_pos = 0;
      }

      _message_arena.reset();
      _shrink_requested = false;
    }
  }

private:
  void _expand()
  {
    size_t const new_capacity = capacity() * 2;
    size_t const current_size = size();

    // This is the only allocation, so a failure throws before any event is moved. The caller
    // relies on the already-cached events surviving so it can keep processing them
    std::vector<TransitEvent> new_storage;
    new_storage.reserve(new_capacity);

    // Move-construct the existing elements into the new storage instead of default-constructing
    // every slot and move-assigning over it. Since the buffer is full, this moves all the previous
    // TransitEvents, preserving their order. The reader position and mask are used to handle the
    // circular buffer's wraparound.
    for (size_t i = 0; i < current_size; ++i)
    {
      new_storage.emplace_back(std::move(_storage[(_reader_pos + i) & _mask]));
    }

    // Only the new tail slots are default-constructed, which does not allocate
    new_storage.resize(new_capacity);

    _storage = std::move(new_storage);
    _mask = new_capacity - 1;
    _popped_events_before_reset += _reader_pos;
    _writer_pos = current_size;
    _reader_pos = 0;
  }

  size_t _initial_capacity;
  size_t _mask;
  size_t _reader_pos{0};
  size_t _writer_pos{0};
  bool _shrink_requested{false};

  // _storage is used as a fixed-size array of ring capacity elements (always a power of two,
  // with _mask being capacity - 1). It is never resized in place; expansion and shrinking
  // replace it with a new vector
  std::vector<TransitEvent> _storage;

  /** The formatted messages of the buffered events, in the order of the events */
  MessageArena _message_arena{message_block_capacity};

  /** _reader_pos starts again from 0 when the storage is replaced */
  size_t _popped_events_before_reset{0};
};

} // namespace detail

QUILL_END_NAMESPACE
