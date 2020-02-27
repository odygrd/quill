#pragma once

#include "quill/detail/misc/Common.h"

#include "quill/detail/LoggerDetails.h"
#include "quill/detail/misc/Os.h"
#include "quill/detail/misc/RdtscClock.h"
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
  RecordBase(RecordBase const&) = delete;
  RecordBase& operator=(RecordBase const&) = delete;
  virtual ~RecordBase() = default;

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
                               RdtscClock const* rdtsc_clock) const noexcept = 0;

private:
#if defined(QUILL_RDTSC_CLOCK)
  uint64_t _timestamp{rdtsc()};
#else
  uint64_t _timestamp{static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count())};
#endif
};
} // namespace detail
} // namespace quill