#include "quill/detail/backend/BacktraceStorage.h"
#include "quill/QuillError.h" // for QUILL_THROW, Quil...
#include "quill/detail/LoggerDetails.h"
#include "quill/detail/misc/Common.h"
#include "quill/detail/misc/Os.h"

namespace quill
{
namespace detail
{

/***/
void BacktraceStorage::store(TransitEvent transit_event)
{
  auto stored_records_it = _stored_records_map.find(transit_event.header.logger_details->name());

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
    stored_object_info.stored_records_collection.emplace_back(std::move(transit_event));
  }
  else
  {
    // Store the object in the vector
    stored_object_info.stored_records_collection[stored_object_info.index] = transit_event;

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

/***/
void BacktraceStorage::process(std::string const& logger_name,
                               std::function<void(TransitEvent const&)> const& callback)
{
  auto stored_records_it = _stored_records_map.find(logger_name);

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
    callback(stored_record_collection[index]);

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

/***/
void BacktraceStorage::set_capacity(std::string const& logger_name, uint32_t capacity)
{
  auto inserted_it =
    _stored_records_map.insert(std::make_pair(std::string{logger_name}, StoredRecordInfo{capacity}));

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
void BacktraceStorage::clear(std::string const& logger_name)
{
  auto stored_records_it = _stored_records_map.find(logger_name);

  if (QUILL_LIKELY(stored_records_it != _stored_records_map.end()))
  {
    // we found stored messages for this logger
    stored_records_it->second.stored_records_collection.clear();
  }
}

} // namespace detail
} // namespace quill