#pragma once

#include <cstdint>
#include <functional>
#include <vector>
#include <x86intrin.h>

#include "quill/detail/LoggerDetails.h"

namespace quill::detail
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
  [[nodiscard]] uint64_t rdtsc() const noexcept { return _rdtsc; }

  /**
   * Required by the queue to get the real record size of the derived class
   * @return The size of the derived class record
   */
  [[nodiscard]] virtual size_t size() const noexcept = 0;

  /**
   * Process a record. Called on the backend worker thread
   * We pass some additional information to this function that are gathered by the backend worker
   * thread
   * @param thread_id the thread_id of the caller thread
   * @param obtain_active_handlers This is a callback to obtain all loggers for use in CommandRecord only
   */
  virtual void backend_process(uint32_t thread_id,
                               std::function<std::vector<Handler*>()> const& obtain_active_handlers) const
    noexcept = 0;

private:
  uint64_t _rdtsc{__rdtsc()};
};

} // namespace quill::detail