#pragma once

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

namespace quill::detail
{

/** forward declaration **/
class ThreadContextCollection;
class ThreadContext;
class MessageBase;

class LoggingWorker
{
public:
  /**
   * Constructor
   */
  LoggingWorker(ThreadContextCollection& thread_context_collection);

  /**
   * Destructor
   */
  ~LoggingWorker();

  /**
   * Returns the status of the logging worker
   * @return true when the worker is running, false otherwise
   */
  [[nodiscard]] bool is_running() const noexcept;

  /**
   * Starts the logging thread
   */
  void run();

  /**
   * Stops the logging thread
   */
  void stop() noexcept;

private:
  /**
   * logging thread main function
   */
  void _main_loop();

  /**
   * Logging thread exist function that flushes everything after stop() is called
   */
  void _exit();

  /**
   * Checks for messages to process
   * @return true if at least one message was found
   */
  [[nodiscard]] bool _check_for_messages(std::vector<ThreadContext*> const& thread_contexts);

private:
  ThreadContextCollection& _thread_context_collection;
  std::atomic<bool> _is_running{false}; /** The spawned logging thread status */

  std::thread _logging_thread;
  std::once_flag _start_init_once_flag;
};
} // namespace quill::detail