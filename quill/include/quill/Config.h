/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Attributes.h"
#include "quill/detail/misc/Common.h"
#include <chrono>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>

namespace quill
{
// forward declarations
class TimestampClock;
class Handler;

struct Config
{
  /** Custom name for the backend thread */
  std::string backend_thread_name = "Quill_Backend";

  /**
   * The backend thread will always "busy wait" spinning around every caller thread's local spsc
   * queue. This option can be enabled to reduce the OS scheduler priority when the backend worker
   * thread is running on a shared cpu. The thread will yield when there is no remaining work to do.
   * @note: When enabled this option will take effect only when backend_thread_sleep_duration is 0
   */
  bool backend_thread_yield = true;

  /**
   * The backend thread will always "busy wait" spinning around every caller thread's local spsc
   * queue. If this value is set then each time the backend thread sees that there are no remaining
   * logs to process in the queues it will sleep.
   */
  std::chrono::nanoseconds backend_thread_sleep_duration = std::chrono::nanoseconds{0};

  /**
   * The backend worker will drain all hot queues and buffer the messages by default.
   * If this option is set to false then it will not buffer and simple process the message
   * with the lowest timestamp from the SPSC queues.
   * @note It is not recommended to set this to false, unless for example you want to limit
   * the logging thread memory usage
   */
  size_t backend_thread_use_transit_buffer = true;

  /**
   * The backend worker thread gives priority to reading the messages from SPSC queues from all
   * the hot threads first and buffers them temporarily.
   *
   * However if the hot threads keep pushing messages to the queues
   * e.g logging in a loop then no logs can ever be processed.
   *
   * When the soft limit is reached then this number of events (default 800) will be logged to the
   * log files before continuing reading the SPSC queues
   *
   * The SPSC queues are emptied on each iteration.
   * This means that the actual messages from the SPSC queues can be much more
   * than the backend_thread_transit_events_soft_limit.
   *
   * @note This number represents a limit across ALL hot threads
   * @note applicable only when backend_thread_use_transit_buffer = true;
   */
  size_t backend_thread_transit_events_soft_limit = 800;

  /**
   * The backend worker thread gives priority to reading the messages from SPSC queues from all
   * the hot threads first and buffers them temporarily.
   *
   * However if the hot threads keep pushing messages to the queues
   * e.g logging in a loop then no logs can ever be processed.
   *
   * As the backend thread is buffering messages it can keep buffering for ever if the hot
   * threads keep pushing.
   *
   * This limit is the maximum size of the backend thread buffer. When reached the backend worker
   * thread will stop reading the SPSC queues until the buffer has space again.
   *
   * @note This is limit PER hot thread
   * @note applicable only when backend_thread_use_transit_buffer = true;
   */
  size_t backend_thread_transit_events_hard_limit = 100'000;

  /**
   * The backend worker thread pops all the SPSC queues log messages and buffers them to a local
   * ring buffer queue as transit events. The transit_event_buffer is unbounded. The initial
   * capacity of the buffer is customisable. Each newly spawned hot thread will have his own
   * transit_event_buffer. This capacity is not in bytes but in items.
   * It must be a power of two.
   * @note applicable only when backend_thread_use_transit_buffer = true;
   */
  uint32_t backend_thread_initial_transit_event_buffer_capacity = 64;

  /**
   * The backend worker thread iterates all the active SPSC queues in order and pops all the messages
   * from each queue. Then it sorts them by timestamp and logs them.
   *
   * Each active queue corresponds to a thread and when multiple threads are logging at the same time
   * it is possible to read a timestamp from the last queue in the iteration but miss that
   * timestamp when the first queue was read because it was not there yet at the time we were reading.
   *
   * When this is enabled the backend worker thread will take a timestamp `now()` before reading
   * the queues. Then it will use that to check each log message timestamp from the active queues is
   * less or equal to the stored 'now()' timestamp ensuring they are ordered by timestamp.
   *
   * Messages that fail the above check are not logged and remain in the queue.
   * They are checked again at the next iteration. Messages are checked on microsecond precision.
   *
   * Enabling this option might delaying popping messages from the SPSC queues.
   * @note applicable only when backend_thread_use_transit_buffer = true;
   */
  bool backend_thread_strict_log_timestamp_order = true;

  /**
   * When this option is enabled and the application is terminating the backend worker thread
   * will not exit until all the SPSC queues are empty. This ensures all messages are logged.
   *
   * However, if during application's destruction there is a thread that keeps trying to log for
   * ever it means that the backend worker thread will have to keep popping log messages and
   * will not be able to exit.
   *
   * When this option is disabled the backend worker thread will try to read the queues once
   * and then exit. Trying only read the queues only once means that some log messages can be
   * dropped especially when backend_thread_strict_log_timestamp_order is set to true.
   */
  bool backend_thread_empty_all_queues_before_exit = true;

  /**
   * Pins the backend thread to the given CPU
   *
   * By default Quill does not pin the backend thread to any CPU, unless a value is specified by
   * this function. max() as undefined value, cpu affinity will not be set
   */
  uint16_t backend_thread_cpu_affinity = (std::numeric_limits<uint16_t>::max)();

  /**
   * Sets the name of the root logger
   */
  std::string default_logger_name = "root";

#if !defined(QUILL_NO_EXCEPTIONS)
  /**
   * The background thread in very rare occasion might thrown an exception which can not be caught
   * in the user threads. In that case the backend worker thread will call this callback instead.
   *
   * Set up a custom error handler to be used if the backend thread has any error.
   *
   * Set an error handler callback e.g :
   *   backend_thread_error_handler = [](std::string const& s) { std::cerr << s << std::endl; }
   */
  backend_worker_error_handler_t backend_thread_error_handler;
#endif

  /**
   * Sets a custom clock that will be used to obtain the timestamp
   * This is useful for example during simulations where you would need to simulate time
   */
  TimestampClock* default_custom_timestamp_clock = nullptr;

  /**
   * Sets the clock type that will be used to obtain the timestamp.
   * Options: rdtsc or system clock
   *
   * - rdtsc mode :
   *
   * TSC clock gives better performance on the caller thread.
   * However, the initialisation time of the application takes longer as we have to take multiple samples
   * in the beginning to convert TSC to nanoseconds
   *
   * When using the TSC counter the backend thread will also periodically call std::chrono::system_clock:now() and will
   * resync the TSC based on the system clock.
   * The backend thread will constantly keep track of the difference between TSC and the system wall clock in order
   * to provide accurate timestamps.
   *
   * - system mode :
   * std::chrono::system_clock:now() is used for obtaining the timestamp
   *
   * By default rdtsc mode is enabled
   *
   * @note You need to have invariant TSC for this mode to work correctly, otherwise please
   * use TimestampClockType::System
   */
  TimestampClockType default_timestamp_clock_type = TimestampClockType::Tsc;

  /**
   * Resets the root logger and re-creates the logger with the given handler
   * This function can also be used to change the format pattern of the logger
   * When the vector is empty then stdout handler is used
   */
  std::vector<std::shared_ptr<Handler>> default_handlers = {};

  /**
   * Enables colours in the terminal.
   * This is only applicable when the default_handlers is empty (the default stdout handler is used then)
   * Otherwise if you set up your own stdout handler with e.g. a custom pattern, then you need to enable the colours
   * yourself. see example_console_colours_with_custom_formatter.cpp and example_console_colours.cpp
   */
  bool enable_console_colours = false;

  /**
   * This option is only applicable if the RDTSC clock is enabled.
   * When system clock is used this option can be ignored
   *
   * This value controls how frequently the backend thread will re-calculate and sync the TSC by
   * getting the system time from the system wall clock.
   * The TSC clock drifts slightly over time and is also not synchronised with the NTP server
   * updates Therefore the smaller this value is the more accurate the log timestamps will be.
   *
   * Decreasing further this value provides more accurate timestamps with the system_clock.
   * Changing this value only affects the backend worker thread performance.
   */
  std::chrono::milliseconds rdtsc_resync_interval = std::chrono::milliseconds{500};

  /**
   * Quill uses an unbounded/bounded SPSC queue per spawned thread to
   * forward the LogRecords to the backend thread.
   *
   * During very high logging activity the backend thread won't be able to consume fast enough
   * and the queue will become full. In this scenario the caller thread will not block but instead
   * it will allocate a new queue of the same capacity.
   *
   * If the backend thread is falling behind also consider reducing the sleep duration of the
   * backend thread first or pinning it to a dedicated core. This will keep the queue more empty.
   *
   * The queue size can be increased or decreased based on the user needs. This queue will be shared
   * between two threads and it should not exceed the size of LLC cache.
   *
   * @warning The configured queue size needs to be in bytes, it MUST be a power of two and a
   * multiple of the page size (4096). e.g. 32'768, 65'536, 131'072, 262'144, 524'288
   *
   * @note This capacity automatically doubles when the unbounded queue is full
   */
  uint32_t default_queue_capacity{131'072};
};
} // namespace quill
