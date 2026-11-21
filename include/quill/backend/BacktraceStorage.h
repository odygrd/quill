/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/backend/MessageArena.h"
#include "quill/backend/TransitEvent.h"
#include "quill/core/Attributes.h"
#include "quill/core/QuillError.h"

#include "quill/bundled/fmt/base.h"

#ifndef QUILL_MODULE
  #include <cstddef>
  #include <cstdint>
  #include <functional>
  #include <string>
  #include <string_view>
  #include <utility>
  #include <vector>
#endif

QUILL_BEGIN_NAMESPACE

namespace detail
{
/**
 * Stores N max messages per logger name in a vector.
 * For simplicity this class is used ONLY by the backend worker thread.
 * We push to the queue a BacktraceCommand event to communicate this from the frontend caller threads
 */
class BacktraceStorage
{
public:
  BacktraceStorage() = default;

  /**
   * Stores a copy of the event and of its formatted message, as the transit event and the memory
   * of its message are reused. When the storage is full the copy replaces the oldest stored event.
   */
  void store(TransitEvent const& transit_event, std::string_view const& thread_id, std::string_view const& thread_name)
  {
    if (_capacity == 0)
    {
      return;
    }

    StoredTransitEvent stored{std::string{thread_id}, std::string{thread_name}, TransitEvent{}};
    transit_event.copy_to(stored.transit_event);

    fmtquill::detail::buffer<char>& formatted_msg = _message_arena.begin_message();
    formatted_msg.append(transit_event.formatted_msg.data(),
                         transit_event.formatted_msg.data() + transit_event.formatted_msg.size());
    stored.transit_event.formatted_msg = _message_arena.commit_message();

    if (_stored_events.size() < _capacity)
    {
      // We are still growing the vector to max capacity, which set_capacity() has reserved
      _stored_events.push_back(std::move(stored));
    }
    else
    {
      // Store the object in the vector, replacing the oldest one, whose message is the oldest
      // of the arena
      _message_arena.pop_front();
      _stored_events[_index] = std::move(stored);

      // Update the index wrapping around the vector capacity
      if (_index < _capacity - 1)
      {
        _index += 1;
      }
      else
      {
        _index = 0;
      }
    }
  }

  /***/
  void process(std::function<void(TransitEvent const&, std::string const& thread_id, std::string const& thread_name)> const& callback)
  {
    // we found stored messages for this logger
    size_t index = _index;

    // Iterate retrieved records in order from first message using index
    QUILL_TRY
    {
      for (size_t i = 0; i < _stored_events.size(); ++i)
      {
        // Give to the user callback the thread id and the RecordBase pointer
        callback(_stored_events[index].transit_event, _stored_events[index].thread_id,
                 _stored_events[index].thread_name);

        // We wrap around to iterate all messages
        if (index < _stored_events.size() - 1)
        {
          index += 1;
        }
        else
        {
          index = 0;
        }
      }
    }
#if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH_ALL()
    {
      // The callback can throw mid-iteration (e.g. a PatternFormatter creation failure); sink
      // write failures are caught per-sink inside the callback and do not propagate here.
      // Clear before propagating, otherwise the records dispatched before the failure would be
      // written again by the next flush. The remaining records are dropped and the error is
      // reported through the backend error notifier
      _clear();
      throw;
    }
#endif

    // finally, clean all messages
    _clear();
  }

  /***/
  void set_capacity(uint32_t capacity)
  {
    if (_capacity != capacity)
    {
      // Reserved first: when it throws the storage is unchanged, and setting the capacity again
      // reserves again. store() relies on the reservation, as it adds an event after its message.
      _stored_events.reserve(capacity);
      _clear();
      _capacity = capacity;
    }
  }

private:
  /** Removes the stored events and frees the memory of their messages */
  void _clear() noexcept
  {
    _stored_events.clear();
    _message_arena.reset();
    _index = 0;
  }

  struct StoredTransitEvent
  {
    StoredTransitEvent(std::string stored_thread_id, std::string stored_thread_name, TransitEvent event)
      : thread_id(std::move(stored_thread_id)),
        thread_name(std::move(stored_thread_name)),
        transit_event(std::move(event))
    {
    }

    /**
     * We use this to take a copy of some objects that are out of scope when a thread finishes but
     * the transit events are still in the buffer
     */
    std::string thread_id;
    std::string thread_name;
    TransitEvent transit_event;
  };

private:
  /** The formatted messages are stored in blocks of this many bytes */
  static constexpr size_t message_block_capacity{4u * 1024u};

  uint32_t _capacity{0};
  uint32_t _index{0};
  std::vector<StoredTransitEvent> _stored_events;

  /** The formatted messages of the stored events, from the oldest event to the newest */
  MessageArena _message_arena{message_block_capacity};
};
} // namespace detail

QUILL_END_NAMESPACE
