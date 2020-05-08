/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Common.h"
#include "quill/detail/LoggerDetails.h"
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
 * Record instances are created automatically by the Logger every time something is logged.
 *
 * They consist of two types:
 * LogRecord - Consisting of a log message to log to the handlers
 * CommandRecord - Consisting of a command to the backend thread. e.g we want to wait until our log is flushed
 *
 * The base record class is used to retrieve and process the correct record types from the queue via the use
 * of virtual functions
 */
class RecordBase
{
public:
  RecordBase() = default;
  virtual ~RecordBase() = default;

  /**
   * Virtual clone
   * @return
   */
  virtual std::unique_ptr<RecordBase> clone() const = 0;

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
   * Process a record. Called on the backend worker thread
   * We pass some additional information to this function that are gathered by the backend worker
   * thread
   * @param thread_id the thread_id of the caller thread
   * @param obtain_active_handlers This is a callback to obtain all loggers for use in CommandRecord only
   */
  virtual void backend_process(char const* thread_id,
                               std::function<std::vector<Handler*>()> const& obtain_active_handlers,
                               std::chrono::nanoseconds log_record_timestamp) const noexcept = 0;

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