/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/BacktraceLogRecordStorage.h"
#include "quill/detail/LoggerDetails.h"
#include "quill/detail/misc/Common.h"
#include "quill/detail/misc/Os.h"
#include "quill/detail/misc/Rdtsc.h"
#include <chrono>
#include <cstdint>
#include <functional>
#include <vector>

namespace quill
{
namespace detail
{
/**
 * Events are pushed to a thread local SPSC queue from the caller threads to the backend thread
 * The base event class is used to retrieve and process the correct event types from the queue via
 * the use of virtual functions
 */
class BaseEvent
{
public:
  BaseEvent() = default;
  virtual ~BaseEvent() = default;

  /**
   * A lambda to obtain all active handlers when processing the log record
   */
  using GetHandlersCallbackT = std::function<std::vector<Handler*>()>;

  /**
   * A lambda to obtain the real timestamp
   */
  using GetRealTsCallbackT = std::function<std::chrono::nanoseconds(BaseEvent const*)>;

  /**
   * Virtual clone
   * @return a copy of record base
   */
  virtual std::unique_ptr<BaseEvent> clone() const = 0;

  /**
   * Get the stored rdtsc timestamp
   * @note Called on the logger thread
   */
  QUILL_NODISCARD uint64_t timestamp() const noexcept { return _timestamp; }

  /**
   * Required by the queue to get the real record size of the derived class
   * @return The size of the derived class record
   */
  QUILL_NODISCARD virtual size_t size() const noexcept = 0;

  /**
   * Process an event. Called on the backend worker thread
   * We pass some additional information to this function that are gathered by the backend worker
   * thread
   * @param backtrace_log_record_storage for backtrace messages
   * @param thread_id the thread_id of the caller thread
   * @param obtain_active_handlers This is a callback to obtain all loggers for use in FlushEvent only
   * @param timestamp_callback a callback to obtain the real timestamp of this event
   */
  virtual void backend_process(BacktraceLogRecordStorage& backtrace_log_record_storage,
                               char const* thread_id, GetHandlersCallbackT const& obtain_active_handlers,
                               GetRealTsCallbackT const& timestamp_callback) const = 0;

  /**
   * Process a backtrace log record. Called on the backend worker thread
   * We pass some additional information to this function that are gathered by the backend worker
   * thread
   */
  virtual void backend_process_backtrace_log_record(char const*, GetHandlersCallbackT const&,
                                                    GetRealTsCallbackT const&) const
  {
    // this is only used for the BacktraceLogRecordEvent and not any other record.
    // We can only store the backtrace LogRecordEvents as BaseEvent* as we need to type erase them
    // and then when we want to process them we call this virtual method.
  }

#if !defined(NDEBUG)
  QUILL_NODISCARD bool using_rdtsc() const noexcept { return _using_rdtsc; }
#endif

private:
#if !defined(QUILL_CHRONO_CLOCK)
  uint64_t _timestamp{rdtsc()};

  #if !defined(NDEBUG)
  bool _using_rdtsc{true};
  #endif

#else
  uint64_t _timestamp{static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count())};

  #if !defined(NDEBUG)
  bool _using_rdtsc{false};
  #endif

#endif
};
} // namespace detail
} // namespace quill