/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/backend/FreeListAllocator.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace quill
{
namespace detail
{

/**
 * Forward Declaration
 */
class BaseEvent;

/**
 * Stores N max messages per logger name in a vector.
 * For simplicity this class is used ONLY by the backend worker thread.
 * We push to the queue a BacktraceCommand event to communicate this from the frontend caller threads
 */
class BacktraceLogRecordStorage
{
public:
  BacktraceLogRecordStorage();
  ~BacktraceLogRecordStorage()
  {
    // we want to clear all messages first, calling deallocate on the free list allocator before destructing
    _stored_records_map.clear();
  }

  /**
   * Stores an object to a vector that maps to logger_name
   * @param logger_name the logger name to store the record
   * @param thread_id the thread id of this record
   * @param record the record to store
   */
  void store(std::string const& logger_name, std::string thread_id, BaseEvent const* record);

  /**
   * Calls the provided callback on all stored objects. The stored objects are provided
   * in order from the first one (oldest record) to the last one (latest record)
   * @param logger_name The name of the logger
   * @param callback A user provided lambda [](std::string const& thread_id, RecordBase const* record) { ... }
   */
  void process(std::string const& logger_name,
               std::function<void(std::string const&, BaseEvent const*)> const& callback);

  /**
   * Insert a new StoredObject with the given capacity
   * @note This has to be called to set a capacity, before a call to store
   * @param logger_name The logger name to set this capacity
   * @param capacity capacity value
   */
  void set_capacity(std::string const& logger_name, uint32_t capacity);

  /**
   * Clears all stored objects for this logger
   * @param logger_name The logger name to clear
   */
  void clear(std::string const& logger_name);

private:
  /**
   * In this type we store a full copy of the thread id (because the thread context can get
   * destroyed before printing the backtrace) and a copy of the Record we would like to log
   */
  struct BacktraceLogRecord
  {
    BacktraceLogRecord(std::string thread_id,
                       std::unique_ptr<BaseEvent, FreeListAllocatorDeleter<BaseEvent>> base_record)
      : thread_id(std::move(thread_id)), base_record(std::move(base_record))
    {
    }

    std::string thread_id;
    std::unique_ptr<BaseEvent, FreeListAllocatorDeleter<BaseEvent>> base_record;
  };

  using StoredRecordsCollection = std::vector<BacktraceLogRecord>;

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

  FreeListAllocator _free_list_allocator; /** The backstore records are stored here */
};
} // namespace detail
} // namespace quill