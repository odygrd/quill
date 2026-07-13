/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/LogLevel.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

#if defined(__MINGW32__)
  #include <iostream>
#endif

QUILL_BEGIN_NAMESPACE

namespace detail
{
inline void backend_options_default_error_notifier(std::string const& error_message)
{
#if !defined(__MINGW32__)
  std::fprintf(stderr, "%s\n", error_message.data());
#else
  // fprintf crashes on mingw gcc 13 for unknown reason
  std::cerr << error_message.data() << "\n";
#endif
}

inline bool backend_options_default_check_printable_char(char c) noexcept
{
  return (c >= ' ' && c <= '~') || (c == '\n') || (c == '\t') || (c == '\r');
}
} // namespace detail

QUILL_BEGIN_EXPORT

/**
 * @brief Configuration options for the backend.
 *
 * This struct defines settings for the backend thread
 */
struct BackendOptions
{
  BackendOptions() = default;

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
  std::chrono::nanoseconds sleep_duration = std::chrono::microseconds{100};

  /**
   * The backend pops all log messages from the frontend queues and buffers them in a
   * local ring buffer queue as transit events. The transit_event_buffer is unbounded, starting with
   * a customizable initial capacity (in items, not bytes) and will reallocate up to
   * transit_events_hard_limit. The backend will use a separate transit_event_buffer for each
   * frontend thread. The capacity is rounded up to the next power of two and the rounded value
   * must not exceed transit_events_hard_limit.
   */
  uint32_t transit_event_buffer_initial_capacity = 256;

  /**
   * The backend gives priority to reading messages from the frontend queues of all
   * the hot threads and temporarily buffers them.
   *
   * If a frontend threads continuously push messages to the queue (e.g., logging in a loop),
   * no logs can ever be processed.
   *
   * When the soft limit is reached the backend worker thread will try to process a batch of cached
   * transit events all at once
   *
   * The frontend queues are emptied on each iteration, so the actual popped messages
   * can be much greater than the transit_events_soft_limit.
   *
   * @note This number represents a limit across the messages received from ALL frontend threads.
   *       It does not need to be a power of two. A value of zero is normalized to one.
   */
  size_t transit_events_soft_limit = 8192;

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
   * @note This number represents a limit PER frontend thread. It must be a power of two. A value
   *       of zero is normalized to one.
   */
  size_t transit_events_hard_limit = 65'536;

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
   * period, reducing the chance of processing events out of timestamp order.
   *
   * Messages that fail the above check remain in the lock-free queue. They are checked again in the
   * next iteration. The timestamp check is performed with microsecond precision.
   *
   * Example scenario:
   * 1. Frontend thread takes a timestamp at the very start of logging, then becomes delayed
   *    (preempted, blocked, processing slowly, etc.) before pushing to the queue.
   * 2. Backend thread takes timestamp `now()` and subtracts the grace period, reads queues up to
   *    the adjusted `now()`, and writes the logs.
   * 3. Frontend thread wakes up and pushes the message with its already-recorded timestamp to the queue.
   * 4. Backend thread reads and writes the delayed timestamp, resulting in an out-of-order log.
   *
   * Setting this option to a non-zero value causes a minor delay in reading messages from the
   * lock-free queues and protects timestamp ordering within the configured grace window. It does
   * not guarantee ordering if a frontend thread records a timestamp and is delayed longer than the
   * grace period before publishing the event to its queue.
   *
   * Setting `log_timestamp_ordering_grace_period` to zero disables this grace-period delay.
   *
   * - 0μs: Fastest processing, may process messages out of timestamp order
   * - 1-5μs: Good default - minimal delay, occasional reordering possible
   * - 10-20μs: Better ordering when threads have different queue arrival timing
   * - 100μs+: Stricter ordering, but risk of SPSC queue filling up at high throughput logging
   *
   * This compensates for timing differences in when threads push to their queues,
   * not for timestamp accuracy itself. The backend also assumes timestamps within a single
   * frontend queue are normally non-decreasing; if a wall clock or user-provided clock moves
   * backwards, `ensure_monotonic_output_timestamps` can correct regular output records.
   *
   * `log_timestamp_ordering_grace_period` is the first line of defense: it delays reading newer
   * queue entries so older timestamped events from other frontend threads have time to arrive.
   * `ensure_monotonic_output_timestamps` is the optional final correction: if an older timestamped
   * regular record still reaches the output stage, it can adjust that record's sink-visible
   * timestamp instead of letting output time move backwards.
   */
  std::chrono::microseconds log_timestamp_ordering_grace_period{5};

  /**
   * Ensures sink-visible timestamps for regular log and metric records do not move backwards.
   *
   * If a regular log or metric record selected for output has a timestamp lower than the last
   * corrected output timestamp, the backend adjusts it to `last_timestamp + 1` nanosecond before
   * formatting, filtering, and writing it to sinks. This is useful when a delayed frontend event
   * escapes the configured grace period or when a clock source moves backwards.
   *
   * Backtrace records are excluded because they intentionally preserve the timestamp from when the
   * backtrace log was captured, even though they are emitted later. Control events such as flush,
   * logger removal, backtrace setup/flush, and MDC changes are also excluded because they are not
   * written as output records.
   *
   * This option changes the timestamp displayed in the output for corrected records. It does not
   * affect frontend timestamp capture or the timestamp used to select the next cached event for
   * processing.
   *
   * This option complements `log_timestamp_ordering_grace_period`. The grace period keeps eligible
   * queue entries back for a bounded amount of time to preserve original timestamps when possible.
   * This option only applies later, after an event has been selected for output, and only when that
   * event would otherwise move sink-visible time backwards.
   */
  bool ensure_monotonic_output_timestamps = false;

  /**
   * When this option is enabled and the application is terminating, the backend worker thread
   * will not exit until all the frontend queues are empty.
   *
   * This assumes frontend threads stop logging before backend shutdown begins. A few trailing log
   * statements may still drain successfully, but you should not rely on that behavior. Sustained
   * concurrent logging while `Backend::stop()` is draining the frontend queues, especially logging
   * in a loop from another thread, can prevent the backend from exiting because the queues may
   * never become empty.
   *
   * When this option is disabled, the backend flushes the sinks and exits without reading the
   * queues again. Log messages still in the queues, as well as messages that were already read
   * from the queues but not yet processed, are discarded.
   */
  bool wait_for_queues_to_empty_before_exit = true;

  /**
   * Pins the backend to the specified CPUs.
   *
   * By default, the backend is not pinned to any CPU unless values are specified.
   * It is recommended to pin the backend to shared non-critical CPUs.
   *
   * @note A failure to apply the affinity is reported through `error_notifier`; in
   *       QUILL_NO_EXCEPTIONS builds it invokes the fatal error path instead.
   * @note On macOS, only the first entry is used because the platform does not expose a per-core
   *       affinity API equivalent to Linux/Windows CPU masks. Apple Silicon does not support the
   *       affinity policy at all, so leave this option empty there.
   * @note On Windows, CPU IDs are relative to the backend thread's current processor group
   *       because this option uses SetThreadAffinityMask. Leave this empty and use a custom
   *       backend-thread hook if your application needs group-aware affinity.
   */
  std::vector<uint16_t> cpu_affinity;

  /**
   * The backend may encounter exceptions that cannot be caught within user threads.
   * In such cases, the backend invokes this callback to notify the user.
   *
   * This function sets up a user error notifier to handle backend errors and notifications,
   * such as when the unbounded queue reallocates or when the bounded queue becomes full.
   *
   * To disable callback notifications, simply leave the function undefined:
   *   std::function<void(std::string const&)> backend_error_notifier = {};
   *
   * Disabling this callback does not suppress Quill's fallback formatting-error entries written
   * to sinks. It only disables the callback notifications themselves.
   *
   * It's safe to perform logging operations within this function (e.g., LOG_INFO(...)).
   * If the backend fills its own blocking frontend queue, additional backend-thread records are
   * dropped because waiting for the backend to consume them would self-deadlock.
   * Calling logger->flush_log(), Backend::stop(), or Frontend::remove_logger_blocking() from the
   * backend thread throws QuillError because the backend cannot wait on itself. If the logger has
   * immediate flush enabled, the implicit flush is silently skipped for backend-thread log calls so
   * generic logging code reused on the backend remains safe.
   */
  std::function<void(std::string const&)> error_notifier{detail::backend_options_default_error_notifier};

  /**
   * Optional hook executed by the backend worker thread at the start of each poll iteration.
   * Any exceptions thrown are forwarded to error_notifier. Useful for thread instrumentation
   * (e.g., Tracy). Use a guard if you need a one-time action. The same backend-thread
   * flush behavior as error_notifier applies.
   */
  std::function<void()> backend_worker_on_poll_begin = {};

  /**
   * Optional hook executed by the backend worker thread at the end of each poll iteration.
   * Any exceptions thrown are forwarded to error_notifier. Useful for thread instrumentation
   * (e.g., Tracy). Use a guard if you need a one-time action. The same backend-thread
   * flush behavior as error_notifier applies.
   */
  std::function<void()> backend_worker_on_poll_end = {};

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
   * This option specifies the minimum time interval (in milliseconds) before the backend thread
   * flushes the output buffers (flush_sink()) for all sinks in the application.
   *
   * The backend thread will ensure that no sink is flushed more frequently than this interval.
   * Explicit calls to `logger->flush_log()` override this interval and trigger an immediate flush.
   *
   * However, if the backend thread is actively processing messages, flushing may occur less
   * frequently than the specified interval.
   *
   * Setting this value to 0 disables the feature, causing the backend thread to flush sinks
   * whenever there is no pending work, provided a write to the sink has occurred.
   *
   * This setting applies globally and affects all sinks in the application.
   */
  std::chrono::milliseconds sink_min_flush_interval = std::chrono::milliseconds{200};

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
  std::function<bool(char c)> check_printable_char{detail::backend_options_default_check_printable_char};

  /**
   * Holds descriptive names for various log levels used in logging operations.
   * The indices correspond to LogLevel enum values defined elsewhere in the codebase.
   * These names provide human-readable identifiers for each log level.
   */
  std::array<std::string, LogLevelCount> log_level_descriptions = {
    "TRACE_L3", "TRACE_L2", "TRACE_L1", "DEBUG",     "INFO", "NOTICE",
    "WARNING",  "ERROR",    "CRITICAL", "BACKTRACE", "NONE"};

  /**
   * @brief Short codes or identifiers for each log level.
   *
   * Provides short codes representing each log level for compact identification and usage.
   * The indices correspond to LogLevel enum values defined elsewhere in the codebase.
   */
  std::array<std::string, LogLevelCount> log_level_short_codes = {"T3", "T2", "T1", "D",  "I", "N",
                                                                  "W",  "E",  "C",  "BT", "_"};

  /**
   * Format string used when rendering PatternFormatter's `%(mdc)`.
   *
   * The string must contain exactly two "{}" placeholders. The text is split into four parts:
   * - prefix: everything before the first "{}"
   * - key-value separator: between the first and second "{}"
   * - field separator: between the second "{}" and the last character
   * - suffix: the last character
   *
   * For example, the default " [{}: {}, ]" renders:
   *   " [request_id: 42, user: alice]"
   *
   * When no MDC is set, %(mdc) expands to an empty string.
   *
   * Invalid patterns are rejected with QuillError during backend initialization.
   */
  std::string mdc_format_pattern = " [{}: {}, ]";

  /**
   * Enables a runtime check to detect multiple instances of the backend singleton.
   *
   * When mixing shared and static libraries, linkage issues can lead to multiple instances
   * of the backend singleton. This may result in multiple backend worker threads running
   * simultaneously, causing unexpected behavior or crashes.
   *
   * This issue commonly occurs on Windows when Quill is compiled as a static library and linked
   * into both a shared library and the main executable, creating separate instances. While using
   * Quill as a static library is generally recommended, in such cases, the preferred approach
   * is to have one shared library own Quill and export/import its shared symbols consistently
   * (for example, define `QUILL_DLL_EXPORT` while building that DLL and `QUILL_DLL_IMPORT`
   * in consumers on Windows).
   *
   * On Windows, this check is implemented using a named mutex, whereas on POSIX systems
   * (Linux, macOS), it uses flock() on a lock file in /tmp. In rare cases, this mechanism
   * may interfere with certain environments or containerized deployments. If necessary,
   * this check can be disabled by setting this option to `false`.
   *
   * Setting this option to `true` enables the check, while setting it to `false` disables it.
   */
  bool check_backend_singleton_instance = true;
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
