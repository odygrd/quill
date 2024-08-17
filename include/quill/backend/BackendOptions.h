/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <limits>
#include <string>

#if defined(__MINGW32__)
  #include <iostream>
#endif

QUILL_BEGIN_NAMESPACE

struct BackendOptions
{
  /**
   * The name assigned to the backend, visible during thread naming queries (e.g.,
   * pthread_getname_np) or in the debugger.
   */
  std::string thread_name = "QuillBackend";

  /**
   * The backend employs "busy-waiting" by spinning around each frontend thread's queue.
   * If enabled, the backend will yield when there is no remaining work, potentially
   * reducing the OS scheduler priority for the backend.
   * This option is effective only when sleep_duration is set to 0.
   */
  bool enable_yield_when_idle = false;

  /**
   * Specifies the duration the backend sleeps if there is no remaining work to process in the queues.
   */
  std::chrono::nanoseconds sleep_duration = std::chrono::nanoseconds{500};

  /**
   * The backend pops all log messages from the frontend queues and buffers them in a
   * local ring buffer queue as transit events. The transit_event_buffer is unbounded, starting with
   * a customizable initial capacity (in items, not bytes) and will reallocate up to
   * transit_events_hard_limit The backend will use a separate transit_event_buffer for each
   * frontend thread. The capacity must be a power of two.
   */
  uint32_t transit_event_buffer_initial_capacity = 64;

  /**
   * The backend gives priority to reading messages from the frontend queues of all
   * the hot threads and temporarily buffers them.
   *
   * If a frontend threads continuously push messages to the queue (e.g., logging in a loop),
   * no logs can ever be processed.
   *
   * When the soft limit is reached (default: 800), the backend worker thread will try to process
   * a batch of cached transit events all at once
   *
   * The frontend queues are emptied on each iteration, so the actual popped messages
   * can be much greater than the transit_events_soft_limit.
   *
   * @note This number represents a limit across the messages received from ALL frontend threads.
   */
  size_t transit_events_soft_limit = 800;

  /**
   * The backend gives priority to reading messages from the frontend queues and temporarily
   * buffers them.
   *
   * If a frontend thread continuously push messages to the queue (e.g., logging in a loop),
   * no logs can ever be processed.
   *
   * As the backend buffers messages, it can keep buffering indefinitely if the frontend
   * threads keep pushing.
   *
   * This limit is the maximum size of the backend event buffer. When reached, the backend
   * will stop reading the frontend queues until there is space available in the buffer.
   *
   * @note This number represents a limit PER frontend threads.
   */
  size_t transit_events_hard_limit = 100'000;

  /**
   * The backend iterates through all frontend lock-free queues and pops all messages from each
   * queue. It then buffers and logs the message with the lowest timestamp among them.
   *
   * Each frontend lock-free queue corresponds to a thread, and when multiple frontend threads are
   * pushing logs simultaneously, it is possible to read a timestamp from the last queue in the
   * iteration but miss that timestamp when the first queue was read because it was not available at
   * that time.
   *
   * When this option is set to a non-zero value, the backend takes a timestamp (`now()`) before
   * reading the queues. It uses that timestamp to ensure that each log message's timestamp from the
   * frontend queues is less than or equal to the stored `now()` timestamp minus the specified grace
   * period, guaranteeing ordering by timestamp.
   *
   * Messages that fail the above check remain in the lock-free queue. They are checked again in the
   * next iteration. The timestamp check is performed with microsecond precision.
   *
   * Example scenario:
   * 1. Frontend thread takes a timestamp, then sleeps before pushing to the queue.
   * 2. Backend thread takes timestamp `now()` and subtracts the grace period, reads queues up to
   *    the adjusted `now()`, and writes the logs.
   * 3. Frontend thread wakes up and pushes to the queue.
   * 4. Backend thread reads and writes the delayed timestamp, resulting in an out-of-order log.
   *
   * Setting this option to a non-zero value causes a minor delay in reading the messages from the
   * lock-free queues but ensures correct timestamp order.
   *
   * Setting `log_timestamp_ordering_grace_period` to zero disables strict timestamp ordering.
   */
  std::chrono::microseconds log_timestamp_ordering_grace_period{1};

  /**
   * When this option is enabled and the application is terminating, the backend worker thread
   * will not exit until all the frontend queues are empty.
   *
   * However, if there is a thread during application destruction that keeps trying to log
   * indefinitely, the backend will be unable to exit because it keeps popping log messages.
   *
   * When this option is disabled, the backend will try to read the queues once
   * and then exit. Reading the queues only once means that some log messages can be dropped,
   * especially when strict_log_timestamp_order is set to true.
   */
  bool wait_for_queues_to_empty_before_exit = true;

  /**
   * Pins the backend to the specified CPU.
   *
   * By default, the backend is not pinned to any CPU unless a value is specified.
   * It is recommended to pin the backend to a shared non-critical CPU.
   * Use `std::numeric_limits<uint16_t>::max()` as an undefined value to avoid setting CPU affinity.
   */
  uint16_t cpu_affinity = (std::numeric_limits<uint16_t>::max)();

  /**
   * The backend may encounter exceptions that cannot be caught within user threads.
   * In such cases, the backend invokes this callback to notify the user.
   *
   * This function sets up a user error notifier to handle backend errors and notifications,
   * such as when the unbounded queue reallocates or when the bounded queue becomes full.
   *
   * To disable notifications, simply leave the function undefined:
   *   std::function<void(std::string const&)> backend_error_notifier = {};
   *
   * It's safe to perform logging operations within this function (e.g., LOG_INFO(...)),
   * but avoid calling logger->flush_log(). The function is invoked on the backend thread,
   * which should not remain in a waiting state as it waits for itself.
   */
  std::function<void(std::string const&)> error_notifier = [](std::string const& error_message)
  {
#if !defined(__MINGW32__)
    std::fprintf(stderr, "%s\n", error_message.data());
#else
    // fprintf crashes on mingw gcc 13 for unknown reason
    std::cerr << error_message.data() << "\n";
#endif
  };

  /**
   * This option is only applicable if at least one frontend is using a Logger with
   * ClockSourceType::Tsc
   *
   * When the system clock is used, this option can be ignored.
   *
   * Controls the frequency at which the backend recalculates and syncs the internal RdtscClock
   * with the system time from the system wall clock.
   * The TSC clock can drift slightly over time and is not synchronized with NTP server updates.
   *
   * Smaller values provide more accurate log timestamps at the cost of additional system clock
   * calls. Changing this value only affects the performance of the backend worker.
   */
  std::chrono::milliseconds rdtsc_resync_interval = std::chrono::milliseconds{500};

  /**
   * This option enables a check that verifies the log message contains only printable characters
   * before forwarding it to the sinks. This adds an extra layer of safety by filtering out
   * non-printable characters from the log file. Any non-printable characters are converted to their
   * equivalent hex value.
   *
   * The check applies only when at least one argument in a log statement is of type string.
   *
   * You can customize this callback to define your own range of printable characters if needed.
   *
   * To disable this check, you can provide:
   *   std::function<bool(char c)> check_printable_char = {}
   */
  std::function<bool(char c)> check_printable_char = [](char c)
  { return (c >= ' ' && c <= '~') || (c == '\n'); };

  /**
   * Holds descriptive names for various log levels used in logging operations.
   * The indices correspond to LogLevel enum values defined elsewhere in the codebase.
   * These names provide human-readable identifiers for each log level.
   */
  std::array<std::string, 12> log_level_descriptions = {
    "TRACE_L3", "TRACE_L2", "TRACE_L1",  "DEBUG", "INFO", "NOTICE", "WARNING",
    "ERROR",    "CRITICAL", "BACKTRACE", "NONE",  "DYNAMIC"};

  /**
   * @brief Short codes or identifiers for each log level.
   *
   * Provides short codes representing each log level for compact identification and usage.
   * The indices correspond to LogLevel enum values defined elsewhere in the codebase.
   */
  std::array<std::string, 11> log_level_short_codes = {"T3", "T2", "T1", "D", "I", "W",
                                                       "E",  "C",  "BT", "N", "DN"};
};

QUILL_END_NAMESPACE