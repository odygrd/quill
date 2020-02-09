#pragma once

#include "quill/detail/Config.h"
#include "quill/detail/misc/RdtscClock.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace quill
{
namespace detail
{
/** forward declaration **/
class ThreadContextCollection;
class ThreadContext;
class RecordBase;
class LoggerCollection;
class HandlerCollection;

class BackendWorker
{
public:
  /**
   * Constructor
   */
  BackendWorker(Config const& config,
                ThreadContextCollection& thread_context_collection,
                HandlerCollection const& handler_collection);

  /**
   * Destructor
   */
  ~BackendWorker();

  /**
   * Returns the status of the backend worker thread
   * @return true when the worker is running, false otherwise
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool is_running() const noexcept;

  /**
   * Starts the backend worker thread
   * @throws
   */
  QUILL_ATTRIBUTE_COLD void run();

  /**
   * Stops the backend worker thread
   */
  QUILL_ATTRIBUTE_COLD void stop() noexcept;

private:
  /**
   * Backend worker thread main function
   */
  QUILL_ATTRIBUTE_HOT void _main_loop();

  /**
   * Logging thread exist function that flushes everything after stop() is called
   */
  QUILL_ATTRIBUTE_COLD void _exit();

  /**
   * Checks for records in all queues and processes the one with the minimum timestamp
   * @return true if one record was found and processed
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool _process_record(std::vector<ThreadContext*> const& thread_contexts);

private:
  /** This is exactly 1 cache line **/
  Config const& _config;
  ThreadContextCollection& _thread_context_collection;
  HandlerCollection const& _handler_collection;
  std::unique_ptr<RdtscClock> _rdtsc_clock; /** rdtsc clock if enabled **/

  std::thread _backend_worker_thread; /** the backend thread that is writing the log to the handlers */
  std::chrono::nanoseconds _backend_thread_sleep_duration; /** backend_thread_sleep_duration from config **/
  std::once_flag _start_init_once_flag; /** flag to start the thread only once, in case start() is called multiple times */
  std::atomic<bool> _is_running{false}; /** The spawned backend thread status */
};
} // namespace detail
} // namespace quill