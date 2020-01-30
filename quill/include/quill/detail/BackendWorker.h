#pragma once

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#include "quill/detail/Config.h"
#include "quill/detail/utility/RdtscClock.h"

namespace quill::detail
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
                LoggerCollection const& logger_collection,
                HandlerCollection const& handler_collection);

  /**
   * Destructor
   */
  ~BackendWorker();

  /**
   * Returns the status of the backend worker thread
   * @return true when the worker is running, false otherwise
   */
  [[nodiscard]] bool is_running() const noexcept;

  /**
   * Starts the backend worker thread
   * @throws
   */
  void run();

  /**
   * Stops the backend worker thread
   */
  void stop() noexcept;

private:
  /**
   * Sets the cpu affinity of the backend thread based on the configured value
   * @throws if fails to set cpu affinity
   */
  void _set_cpu_affinity() const;

  /**
   * Sets the name of the thread
   * @throws
   */
  void _set_thread_name() const;

  /**
   * Backend worker thread main function
   */
  void _main_loop();

  /**
   * Logging thread exist function that flushes everything after stop() is called
   */
  void _exit();

  /**
   * Checks for records in all queues and processes the one with the minimum timestamp
   * @return true if one record was found and processed
   */
  [[nodiscard]] bool _process_record(std::vector<ThreadContext*> const& thread_contexts);

private:
  Config const& _config;
  ThreadContextCollection& _thread_context_collection;
  LoggerCollection const& _logger_collection;
  HandlerCollection const& _handler_collection;
  RdtscClock _rdtsc_clock;

  std::thread _backend_worker_thread; /** the backend thread that is writing the log to the handlers */
  std::once_flag _start_init_once_flag; /** flag to start the thread only once, in case start() is called multiple times */
  std::chrono::nanoseconds _backend_thread_sleep_duration; /** backend_thread_sleep_duration from config **/
  std::atomic<bool> _is_running{false};                    /** The spawned backend thread status */
};

} // namespace quill::detail