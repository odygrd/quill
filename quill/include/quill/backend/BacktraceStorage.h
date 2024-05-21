/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/backend/TransitEvent.h"
#include "quill/core/Attributes.h"
#include "quill/core/LoggerBase.h"
#include "quill/core/QuillError.h"

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace quill::detail
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
  ~BacktraceStorage()
  {
    // we want to clear all messages first, calling deallocate on the free list allocator before destructing
    _stored_records_map.clear();
  }

  /**
   * Stores an object to a vector that maps to logger_name
   * @param transit_event transit_event
   */
  void store(TransitEvent transit_event)
  {
    auto const stored_records_it = _stored_records_map.find(transit_event.logger_base->get_logger_name());

    if (QUILL_UNLIKELY(stored_records_it == _stored_records_map.end()))
    {
      // We have never used backtrace this logger name before, need to call set capacity first
      QUILL_THROW(QuillError{
        "logger->init_backtrace(...) needs to be called first before using LOG_BACKTRACE(...)."});
    }

    // we found a stored vector for this logger name and we have to update it
    StoredRecordInfo& stored_object_info = stored_records_it->second;

    if (stored_object_info.stored_records_collection.size() < stored_object_info.capacity)
    {
      // We are still growing the vector to max capacity
      auto& emplaced = stored_object_info.stored_records_collection.emplace_back(
        std::string{transit_event.thread_name}, std::string{transit_event.thread_id}, std::move(transit_event));

      // we want to point the transit event objects to ours because they can point to invalid memory
      // if the thread is destructed
      emplaced.transit_event.thread_name = emplaced.thread_name;
      emplaced.transit_event.thread_id = emplaced.thread_id;
    }
    else
    {
      // Store the object in the vector
      StoredTransitEvent& ste = stored_object_info.stored_records_collection[stored_object_info.index];
      ste = StoredTransitEvent{std::string{transit_event.thread_name},
                               std::string{transit_event.thread_id}, std::move(transit_event)};

      // we want to point the transit event objects to ours because they can point to invalid memory
      // if the thread is destructed
      ste.transit_event.thread_name = ste.thread_name;
      ste.transit_event.thread_id = ste.thread_id;

      // Update the index wrapping around the vector capacity
      if (stored_object_info.index < stored_object_info.capacity - 1)
      {
        stored_object_info.index += 1;
      }
      else
      {
        stored_object_info.index = 0;
      }
    }
  }

  /**
   * Calls the provided callback on all stored objects. The stored objects are provided
   * in order from the first one (oldest record) to the last one (latest record)
   */
  void process(std::string const& logger_name, std::function<void(TransitEvent const&)> const& callback)
  {
    auto const stored_records_it = _stored_records_map.find(logger_name);

    if (QUILL_UNLIKELY(stored_records_it == _stored_records_map.end()))
    {
      // Not found, Nothing to iterate
      return;
    }

    // we found stored messages for this logger
    uint32_t index = stored_records_it->second.index;
    StoredRecordsCollection& stored_record_collection = stored_records_it->second.stored_records_collection;
    // Iterate retrieved records in order from first message using index
    for (uint32_t i = 0; i < stored_record_collection.size(); ++i)
    {
      // Give to the user callback the thread id and the RecordBase pointer
      callback(stored_record_collection[index].transit_event);

      // We wrap around to iterate all messages
      if (index < stored_record_collection.size() - 1)
      {
        index += 1;
      }
      else
      {
        index = 0;
      }
    }

    // finally clean all messages
    stored_record_collection.clear();
  }

  /**
   * Insert a new StoredObject with the given capacity
   * @note This has to be called to set a capacity, before a call to store
   * @param logger_name The logger name to set this capacity
   * @param capacity capacity value
   */
  void set_capacity(std::string const& logger_name, uint32_t capacity)
  {
    auto inserted_it = _stored_records_map.emplace(std::string{logger_name}, StoredRecordInfo{capacity});

    if (!inserted_it.second)
    {
      StoredRecordInfo& stored_object_info = inserted_it.first->second;

      // We could not insert, the user called set_backtrace_capacity for a
      // second time while one was active. In this case we will clear the existing one and
      // store the new capacity if the new capacity is different
      if (stored_object_info.capacity != capacity)
      {
        stored_object_info.stored_records_collection.clear();
        stored_object_info.capacity = capacity;
      }
    }
  }

  /***/
  void erase(std::string const& logger_name) { _stored_records_map.erase(logger_name); }

private:
  /**
   * We use this to take o copy of some objects that are out of space when a thread finishes but
   * the transit events are still in the buffer
   */
  struct StoredTransitEvent
  {
    StoredTransitEvent(std::string thread_name, std::string thread_id, TransitEvent transit_event)
      : thread_name(std::move(thread_name)),
        thread_id(std::move(thread_id)),
        transit_event(std::move(transit_event))
    {
    }

    std::string thread_name;
    std::string thread_id;
    TransitEvent transit_event;
  };

  using StoredRecordsCollection = std::vector<StoredTransitEvent>;

  /**
   * We map each logger name to this type
   */
  struct StoredRecordInfo
  {
    explicit StoredRecordInfo(uint32_t capacity) : capacity(capacity)
    {
      // also reserve the vector
      stored_records_collection.reserve(capacity);
    }

    uint32_t capacity;                                   /** The maximum capacity for this */
    uint32_t index{0};                                   /** Our current index */
    StoredRecordsCollection stored_records_collection{}; /** A vector holding stored objects */
  };

private:
  /** A map where we store a vector of stored records for each logger name. We use the vectors like a ring buffer and loop around */
  std::unordered_map<std::string, StoredRecordInfo> _stored_records_map;
};
} // namespace quill::detail