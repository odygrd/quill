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
   * The backend thread will always "busy wait" spinning around every caller thread's local spsc queue.
   * To reduce the OS scheduler priority when the backend worker thread is running on a shared cpu, they thread will attempt a periodic call to sleep().
   * Each time the backend thread sees that there are no remaining logs to process in the queues it will sleep.
   */
  std::chrono::nanoseconds backend_thread_sleep_duration = std::chrono::nanoseconds{300};

  /**
   * The backend worker thread gives priority to reading the messages from SPSC queues from all the hot threads first,
   * and storing them temporarily as transit events.
   * However if the hot threads keep pushing messages to the queues e.g logging in a loop then no logs can ever be
   * processed
   * This variable sets the maximum transit events number. When that number is reached then half of them will get
   * flushed to the log files before continuing reading the SPSC queues
   */
  size_t backend_thread_max_transit_events = 800;

  /**
   * Pins the backend thread to the given CPU
   *
   * By default Quill does not pin the backend thread to any CPU, unless a value is specified by
   * this function. max() as undefined value, cpu affinity will not be set
   */
  uint16_t backend_thread_cpu_affinity = (std::numeric_limits<uint16_t>::max)();

  /**
   * Sets the name of the default logger
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
   * @param timestamp_clock a custom class deriving from TimestampClock
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
   */
  TimestampClockType default_timestamp_clock_type = TimestampClockType::Rdtsc;

  /**
   * Resets the default logger and re-creates the logger with the given handler
   * * This function can also be used to change the format pattern of the logger
   * * When the vector is empty then stdout handler is used
   * Any loggers that are created by using create_logger(std::string logger_name) use the same handler by default
   */
  std::vector<Handler*> default_handlers = {};

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
   * It is not recommended to change the default value unless there is a real reason.
   * The value is in milliseconds and the default value is 700.
   */
  std::chrono::milliseconds rdtsc_resync_interval = std::chrono::milliseconds{700};
};
} // namespace quill