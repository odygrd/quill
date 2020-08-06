/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/record/RecordBase.h"
#include <functional>
#include <limits>

namespace quill
{
namespace detail
{
/**
 * Special type of command record.
 * We use this class to control the backtrace from the caller threads
 */
class BacktraceCommand final : public RecordBase
{
public:
  /**
   * In this constructor if we want to pass _backtrace_capacity we Setup the backtrace
   * If we leave _backtrace_capacity as default then we flush the backtrace
   * @param command
   * @param logger_details
   * @param backtrace_capacity
   */
  BacktraceCommand(LoggerDetails const* logger_details,
                   uint32_t backtrace_capacity = (std::numeric_limits<uint32_t>::max()))
    : _logger_details(logger_details), _backtrace_capacity(backtrace_capacity)
  {
  }

  QUILL_NODISCARD std::unique_ptr<RecordBase> clone() const override
  {
    return std::make_unique<BacktraceCommand>(*this);
  }

  QUILL_NODISCARD size_t size() const noexcept override { return sizeof(*this); }

  /**
   * When we encounter this message we are going to either insert a new backtrace for a logger or print an existing backtrace
   * @param obtain_active_handlers a function that is passed to this method and obtains all the active handlers when called
   * @timestamp_callback a callback to obtain the timestamp
   */
  void backend_process(BacktraceRecordStorage& backtrace_record_storage, char const*,
                       GetHandlersCallbackT const& obtain_active_handlers,
                       GetRealTsCallbackT const& timestamp_callback) const noexcept override
  {
    if (_backtrace_capacity != std::numeric_limits<uint32_t>::max())
    {
      // If the _backtrace_capacity is set it means we are seting up the backtrace
      // We setup the backtrace like this with an event to the backend thread - that way we avoid any overcomplications
      backtrace_record_storage.set_capacity(_logger_details->name(), _backtrace_capacity);
    }
    else
    {
      // process all records in backtrace for this logger_name and log them by calling backend_process_backtrace_record
      backtrace_record_storage.process(
        _logger_details->name(),
        [&obtain_active_handlers, &timestamp_callback](std::string const& stored_thread_id,
                                                       RecordBase const* stored_backtrace_record) {
          stored_backtrace_record->backend_process_backtrace_record(
            stored_thread_id.data(), obtain_active_handlers, timestamp_callback);
        });
    }
  }

  /**
   * Destructor
   */
  ~BacktraceCommand() override = default;

private:
  LoggerDetails const* _logger_details{nullptr};
  uint32_t _backtrace_capacity{(std::numeric_limits<uint32_t>::max())};
};

} // namespace detail
} // namespace quill