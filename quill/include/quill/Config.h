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
  /**
   * Custom name for the backend thread.
   */
  std::string backend_thread_name = "Quill_Backend";

  /**
   * Determines whether the backend thread will "busy wait" by spinning around every caller thread's
   * local spsc queue. If enabled, this option reduces the OS scheduler priority when the backend
   * worker thread is running on a shared CPU. The thread will yield when there is no remaining work
   * to do.
   * @note This option only takes effect when backend_thread_sleep_duration is set to 0.
   */
  bool backend_thread_yield = false;

  /**
   * Determines the duration for which the backend thread will "busy wait" by spinning around every
   * caller thread's local spsc queue. If a value is set, each time the backend thread sees that
   * there are no remaining logs to process in the queues, it will sleep for the specified duration.
   */
  std::chrono::nanoseconds backend_thread_sleep_duration = std::chrono::nanoseconds{500};

  /**
   * Determines the behavior of the backend worker thread. By default, it will drain all hot queues and buffer the
   * messages. If this option is set to false, the backend thread will simply process the message with the lowest
   * timestamp from the SPSC queues without buffering.
   * @note It is generally not recommended to set this to false, unless you want to limit the logging thread's memory usage.
   */
  size_t backend_thread_use_transit_buffer = true;

  /**
   * The backend worker thread gives priority to reading messages from the SPSC queues of all
   * the hot threads and temporarily buffers them.
   *
   * If the hot threads continuously push messages to the queues (e.g., logging in a loop),
   * no logs can ever be processed.
   *
   * When the soft limit is reached (default: 800), this number of events will be logged to the
   * log files before continuing to read the SPSC queues.
   *
   * The SPSC queues are emptied on each iteration, so the actual messages from the SPSC queues
   * can be much greater than the backend_thread_transit_events_soft_limit.
   *
   * @note This number represents a limit across ALL hot threads.
   * @note Applicable only when backend_thread_use_transit_buffer = true.
   */
  size_t backend_thread_transit_events_soft_limit = 800;

  /**
   * The backend worker thread gives priority to reading messages from the SPSC queues of all
   * the hot threads and temporarily buffers them.
   *
   * If the hot threads continuously push messages to the queues (e.g., logging in a loop),
   * no logs can ever be processed.
   *
   * As the backend thread buffers messages, it can keep buffering indefinitely if the hot
   * threads keep pushing.
   *
   * This limit is the maximum size of the backend thread buffer. When reached, the backend worker
   * thread will stop reading the SPSC queues until there is space available in the buffer.
   *
   * @note This limit applies PER hot thread.
   * @note Applicable only when backend_thread_use_transit_buffer = true.
   */
  size_t backend_thread_transit_events_hard_limit = 100'000;

  /**
   * The backend worker thread pops all log messages from the SPSC queues and buffers them in a
   * local ring buffer queue as transit events. The transit_event_buffer is unbounded, with a
   * customizable initial capacity (in items, not bytes). Each newly spawned hot thread will have
   * its own transit_event_buffer. The capacity must be a power of two.
   *
   * @note Applicable only when backend_thread_use_transit_buffer = true.
   */
  uint32_t backend_thread_initial_transit_event_buffer_capacity = 64;

  /**
   * The backend worker thread iterates through all active SPSC queues and pops all messages from each queue.
   * It then sorts the messages by timestamp and logs them.
   *
   * Each active queue corresponds to a thread, and when multiple threads are logging simultaneously,
   * it is possible to read a timestamp from the last queue in the iteration but miss that
   * timestamp when the first queue was read because it was not available at that time.
   *
   * When this option is enabled, the backend worker thread takes a timestamp (`now()`) before reading
   * the queues. It uses that timestamp to ensure that each log message's timestamp from the active queues
   * is less than or equal to the stored `now()` timestamp, guaranteeing ordering by timestamp.
   *
   * Messages that fail the above check are not logged and remain in the queue.
   * They are checked again in the next iteration. The timestamp check is performed with microsecond precision.
   *
   * Enabling this option may cause a delay in popping messages from the SPSC queues.
   * @note Applicable only when backend_thread_use_transit_buffer = true.
   */
  bool backend_thread_strict_log_timestamp_order = true;

  /**
   * When this option is enabled and the application is terminating, the backend worker thread
   * will not exit until all the SPSC queues are empty. This ensures that all messages are logged.
   *
   * However, if there is a thread during application destruction that keeps trying to log indefinitely,
   * the backend worker thread will be unable to exit because it keeps popping log messages.
   *
   * When this option is disabled, the backend worker thread will try to read the queues once
   * and then exit. Reading the queues only once means that some log messages can be dropped,
   * especially when backend_thread_strict_log_timestamp_order is set to true.
   */
  bool backend_thread_empty_all_queues_before_exit = true;

  /**
   * Pins the backend thread to the specified CPU.
   *
   * By default, Quill does not pin the backend thread to any CPU unless a value is specified.
   * Use `std::numeric_limits<uint16_t>::max()` as an undefined value to avoid setting CPU affinity.
   */
  uint16_t backend_thread_cpu_affinity = (std::numeric_limits<uint16_t>::max)();

  /**
   * Sets the name of the root logger.
   */
  std::string default_logger_name = "root";

  /**
   * The background thread might occasionally throw an exception that cannot be caught in the user threads.
   * In that case, the backend worker thread will call this callback instead.
   *
   * Set up a custom notification handler to be used if the backend thread encounters any error.
   * This handler is also used to deliver messages to the user, such as when the unbounded queue reallocates
   * or when the bounded queue becomes full.
   *
   * When not set here, the default is:
   *   backend_thread_notification_handler = [](std::string const& s) { std::cerr << s << std::endl; }
   *
   * To disable notifications, use:
   *   backend_thread_notification_handler = [](std::string const&) { }
   */
  backend_worker_notification_handler_t backend_thread_notification_handler;

  /**
   * Sets a custom clock that will be used to obtain the timestamp.
   * This is useful, for example, during simulations where you need to simulate time.
   */
  TimestampClock* default_custom_timestamp_clock = nullptr;

  /**
   * Sets the clock type that will be used to obtain the timestamp.
   * Options: rdtsc or system clock.
   *
   * - rdtsc mode:
   *   TSC clock provides better performance on the caller thread.
   *   However, the initialization time of the application is longer as multiple samples need to be taken
   *   in the beginning to convert TSC to nanoseconds.
   *
   *   When using the TSC counter, the backend thread will periodically call `std::chrono::system_clock::now()`
   *   to resync the TSC based on the system clock.
   *   The backend thread constantly keeps track of the difference between TSC and the system wall clock
   *   to provide accurate timestamps.
   *
   * - system mode:
   *   `std::chrono::system_clock::now()` is used to obtain the timestamp.
   *
   * By default, rdtsc mode is enabled.
   *
   * @note You need to have an invariant TSC for this mode to work correctly. Otherwise, use `TimestampClockType::System`.
   */
  TimestampClockType default_timestamp_clock_type = TimestampClockType::Tsc;

  /**
   * Resets the root logger and recreates the logger with the given handler.
   * This function can also be used to change the format pattern of the logger.
   * If the vector is empty, the stdout handler is used by default.
   */
  std::vector<std::shared_ptr<Handler>> default_handlers = {};

  /**
   * Enables colors in the terminal.
   * This option is only applicable when the default_handlers vector is empty (the default stdout handler is used).
   * If you set up your own stdout handler with a custom pattern, you need to enable the colors yourself.
   * See example_console_colours_with_custom_formatter.cpp and example_console_colours.cpp for examples.
   */
  bool enable_console_colours = false;

  /**
   * This option is only applicable if the RDTSC clock is enabled.
   * When the system clock is used, this option can be ignored.
   *
   * Controls the frequency at which the backend thread recalculates and syncs the TSC by
   * obtaining the system time from the system wall clock.
   * The TSC clock drifts slightly over time and is not synchronized with NTP server updates.
   * A smaller value results in more accurate log timestamps.
   * Decreasing this value further provides more accurate timestamps with the system_clock.
   * Changing this value only affects the performance of the backend worker thread.
   */
  std::chrono::milliseconds rdtsc_resync_interval = std::chrono::milliseconds{500};

  /**
   * Quill uses an unbounded/bounded SPSC queue per spawned thread to forward the log messages to
   * the backend thread. During high logging activity, if the backend thread cannot consume logs
   * fast enough, the queue may become full. In this scenario, the caller thread does not block but
   * instead allocates a new queue with the same capacity. If the backend thread is falling behind,
   * consider reducing the sleep duration of the backend thread or pinning it to a dedicated core to
   * keep the queue less congested. The queue size can be increased or decreased based on user
   * needs. The queue is shared between two threads and should not exceed the size of the LLC cache.
   *
   * @warning The configured queue size must be in bytes, a power of two, and a multiple of the page
   * size (4096). For example: 32,768; 65,536; 131,072; 262,144; 524,288.
   * @note This capacity automatically doubles when the unbounded queue is full.
   */
  uint32_t default_queue_capacity{131'072};

  /**
   * When set to true, enables huge pages for all queue allocations on the hot path.
   * Make sure you have huge pages enabled on your Linux system for this to work.
   *
   * To check if huge pages are enabled:
   *   cat /proc/meminfo | grep HugePages
   *
   * To set the number of huge pages:
   *   sudo sysctl -w vm.nr_hugepages=<number_of_hugepages>
   *
   * @note This option is only supported on Linux.
   */
  bool enable_huge_pages_hot_path{false};
};
} // namespace quill
