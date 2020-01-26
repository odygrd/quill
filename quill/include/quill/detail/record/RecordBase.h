#pragma once

#include <cstdint>

#include <x86intrin.h>

namespace quill::detail
{
/**
 * Record instances are created automatically by the Logger every time something is logged.
 *
 * They consist of two types:
 * LogRecord - Consisting of a log message to log to the sinks
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
   * @param thread_id the thread_id of the caller thread
   */
  virtual void backend_process(uint32_t thread_id) const noexcept = 0;

private:
  uint64_t _rdtsc{__rdtsc()};
};

} // namespace quill::detail