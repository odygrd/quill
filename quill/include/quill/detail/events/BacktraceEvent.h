/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/events/BaseEvent.h"
#include <functional>
#include <limits>

namespace quill
{
namespace detail
{
/**
 * Special type of event used to enable or flush a backtrace
 * We use this type of event to control the backtrace from the caller threads
 */
class BacktraceEvent final : public BaseEvent
{
public:
  /**
   * In this constructor if we want to pass _backtrace_capacity we setup the backtrace
   * If we leave _backtrace_capacity as default then we flush the backtrace
   * @param logger_details the logger sending this command
   * @param backtrace_capacity provide backtrace_capacity only if we want to setup the backtrace
   */
  BacktraceEvent(LoggerDetails const* logger_details,
                 uint32_t backtrace_capacity = (std::numeric_limits<uint32_t>::max)())
    : _logger_details(logger_details), _backtrace_capacity(backtrace_capacity)
  {
  }

  /**
   * Virtual clone using a custom memory manager
   * @return a copy of this object
   */
  QUILL_NODISCARD std::unique_ptr<BaseEvent, FreeListAllocatorDeleter<BaseEvent>> clone(FreeListAllocator& fla) const override
  {
    // allocate memory using the memory manager
    void* buffer = fla.allocate(sizeof(BacktraceEvent));

    // create emplace a new object inside the buffer using the copy constructor of LogEvent
    // and store this in a unique ptr with the custom deleter
    return std::unique_ptr<BaseEvent, FreeListAllocatorDeleter<BaseEvent>>{
      new (buffer) BacktraceEvent(*this), FreeListAllocatorDeleter<BaseEvent>{&fla}};
  }

  QUILL_NODISCARD size_t size() const noexcept override { return sizeof(*this); }

  /**
   * When we encounter this message we are going to either insert a new backtrace for a logger or print an existing backtrace
   * @param obtain_active_handlers a function that is passed to this method and obtains all the active handlers when called
   * @param timestamp_callback a callback to obtain the timestamp
   */
  void backend_process(BacktraceLogRecordStorage& backtrace_log_record_storage, char const*,
                       GetHandlersCallbackT const& obtain_active_handlers,
                       GetRealTsCallbackT const& timestamp_callback) const noexcept override
  {
    if (_backtrace_capacity != (std::numeric_limits<uint32_t>::max)())
    {
      // If the _backtrace_capacity is set it means we are setting up the backtrace
      // We setup the backtrace like this with an event to the backend thread - that way we avoid any overcomplications
      backtrace_log_record_storage.set_capacity(_logger_details->name(), _backtrace_capacity);
    }
    else
    {
      // process all records in backtrace for this logger_name and log them by calling backend_process_backtrace_log_record
      backtrace_log_record_storage.process(
        _logger_details->name(),
        [&obtain_active_handlers, &timestamp_callback](
          std::string const& stored_thread_id, BaseEvent const* stored_backtrace_log_record) {
          stored_backtrace_log_record->backend_process_backtrace_log_record(
            stored_thread_id.data(), obtain_active_handlers, timestamp_callback);
        });
    }
  }

  /**
   * Destructor
   */
  ~BacktraceEvent() override = default;

private:
  LoggerDetails const* _logger_details{nullptr};
  uint32_t _backtrace_capacity{(std::numeric_limits<uint32_t>::max)()};
};

} // namespace detail
} // namespace quill