#include "quill/Quill.h"
#include "quill/QuillError.h"
#include "quill/detail/Config.h"                    // for Config
#include "quill/detail/HandlerCollection.h"         // for HandlerCollection
#include "quill/detail/LogManager.h"                // for LogManagerSingleton
#include "quill/detail/LoggerCollection.h"          // for LoggerCollection
#include "quill/detail/ThreadContext.h"             // for ThreadContext, Thr...
#include "quill/detail/ThreadContextCollection.h"   // for ThreadContextColle...
#include "quill/handlers/ConsoleHandler.h"          // for ConsoleHandler
#include "quill/handlers/FileHandler.h"             // for FileHandler, Filenam...
#include "quill/handlers/RotatingFileHandler.h"     // for RotatingFileHandler
#include "quill/handlers/StreamHandler.h"           // for StreamHandler
#include "quill/handlers/TimeRotatingFileHandler.h" // for TimeRotatingFileHandler
#include <utility>                                  // for move

namespace quill
{
/***/
void preallocate()
{
  QUILL_MAYBE_UNUSED size_t const volatile x = detail::LogManagerSingleton::instance()
                                                 .log_manager()
                                                 .thread_context_collection()
                                                 .local_thread_context()
                                                 ->spsc_queue()
                                                 .capacity();
}

/***/
Handler* stdout_handler(std::string const& stdout_handler_name /* = "stdout" */,
                        ConsoleColours const& console_colours /* = ConsoleColours {} */)
{
  if (console_colours.using_colours())
  {
    // if we are using colours then expect a different name from the default "stdout"
    if (stdout_handler_name == std::string{"stdout"})
    {
      QUILL_THROW(
        QuillError{"When using colours a different handler name than 'stdout' needs to be used"});
    }
  }

  return detail::LogManagerSingleton::instance().log_manager().handler_collection().stdout_console_handler(
    stdout_handler_name, console_colours);
}

/***/
Handler* stderr_handler(std::string const& stderr_handler_name /* = "stderr" */)
{
  return detail::LogManagerSingleton::instance().log_manager().handler_collection().stderr_console_handler(
    stderr_handler_name);
}

/***/
Handler* file_handler(fs::path const& filename, std::string const& mode, /* = std::string{} */
                      FilenameAppend append_to_filename /* = FilenameAppend::None */)
{
  return create_handler<FileHandler>(filename.string(), mode, append_to_filename);
}

/***/
Handler* time_rotating_file_handler(fs::path const& base_filename,
                                    std::string const& mode /* = std::string{"a"} */,
                                    std::string const& when /* = std::string{"H"} */,
                                    uint32_t interval /* = 1 */, uint32_t backup_count /* = 0 */,
                                    Timezone timezone /* = Timezone::LocalTime */,
                                    std::string const& at_time /* = std::string{} */)
{
  return create_handler<TimeRotatingFileHandler>(base_filename.string(), mode, when, interval,
                                                 backup_count, timezone, at_time);
}

/***/
Handler* rotating_file_handler(fs::path const& base_filename,
                               std::string const& mode /* = std::string {"a"} */, size_t max_bytes /* = 0 */,
                               uint32_t backup_count /* = 0 */, bool overwrite_oldest_files /* = true */)
{
  return create_handler<RotatingFileHandler>(base_filename.string(), mode, max_bytes, backup_count,
                                             overwrite_oldest_files);
}

/***/
Logger* get_logger(char const* logger_name /* = nullptr */)
{
  return detail::LogManagerSingleton::instance().log_manager().logger_collection().get_logger(logger_name);
}

/***/
std::unordered_map<std::string, Logger*> get_all_loggers()
{
  return detail::LogManagerSingleton::instance().log_manager().logger_collection().get_all_loggers();
}

/***/
Logger* create_logger(std::string const& logger_name,
                      std::optional<TimestampClockType> timestamp_clock_type /* = std::nullopt */,
                      std::optional<TimestampClock*> timestamp_clock /* = std::nullopt */)
{
  return detail::LogManagerSingleton::instance().log_manager().create_logger(
    logger_name, timestamp_clock_type, timestamp_clock);
}

/***/
Logger* create_logger(std::string const& logger_name, Handler* handler,
                      std::optional<TimestampClockType> timestamp_clock_type /* = std::nullopt */,
                      std::optional<TimestampClock*> timestamp_clock /* = std::nullopt */)
{
  return detail::LogManagerSingleton::instance().log_manager().create_logger(
    logger_name, handler, timestamp_clock_type, timestamp_clock);
}

/***/
Logger* create_logger(std::string const& logger_name, std::initializer_list<Handler*> handlers,
                      std::optional<TimestampClockType> timestamp_clock_type /* = std::nullopt */,
                      std::optional<TimestampClock*> timestamp_clock /* = std::nullopt */)
{
  return detail::LogManagerSingleton::instance().log_manager().create_logger(
    logger_name, handlers, timestamp_clock_type, timestamp_clock);
}

/***/
Logger* create_logger(std::string const& logger_name, std::vector<Handler*> const& handlers,
                      std::optional<TimestampClockType> timestamp_clock_type /* = std::nullopt */,
                      std::optional<TimestampClock*> timestamp_clock /* = std::nullopt */)
{
  return detail::LogManagerSingleton::instance().log_manager().create_logger(
    logger_name, handlers, timestamp_clock_type, timestamp_clock);
}

/***/
void set_timestamp_clock_type(TimestampClockType timestamp_clock_type)
{
  if (detail::LogManagerSingleton::instance().log_manager().backend_worker_is_running())
  {
    QUILL_THROW(
      QuillError{"quill::set_timestamp_clock_type(...) needs to be called before quill::start()."});
  }

  detail::LogManagerSingleton::instance().log_manager().set_timestamp_clock_type(timestamp_clock_type);
}

/***/
void set_custom_timestamp_clock(TimestampClock* timestamp_clock)
{
  if (detail::LogManagerSingleton::instance().log_manager().backend_worker_is_running())
  {
    QUILL_THROW(QuillError{
      "quill::set_custom_timestamp_clock(...) needs to be called before quill::start()."});
  }

  detail::LogManagerSingleton::instance().log_manager().set_custom_timestamp_clock(timestamp_clock);
}

/***/
void set_default_logger_handler(Handler* handler)
{
  if (detail::LogManagerSingleton::instance().log_manager().backend_worker_is_running())
  {
    QUILL_THROW(QuillError{
      "quill::set_default_logger_handler(...) needs to be called before quill::start(). That can "
      "cause a race condition on the backend worker thread. Catch the exception to "
      "avoid this error but the call to quill::set_default_logger_handler(...) will have no "
      "effect."});
  }

  detail::LogManagerSingleton::instance().log_manager().set_default_logger_handler(handler);
}

/***/
void set_default_logger_handler(std::initializer_list<Handler*> handlers)
{
  if (detail::LogManagerSingleton::instance().log_manager().backend_worker_is_running())
  {
    QUILL_THROW(QuillError{
      "quill::set_default_logger_handler(...) needs to be called before quill::start(). That can "
      "cause a race condition on the backend worker thread. Catch the exception to "
      "avoid this error but the call to quill::set_default_logger_handler(...) will have no "
      "effect."});
  }

  detail::LogManagerSingleton::instance().log_manager().set_default_logger_handler(handlers);
}

/***/
void enable_console_colours()
{
  if (detail::LogManagerSingleton::instance().log_manager().backend_worker_is_running())
  {
    QUILL_THROW(QuillError{
      "quill::enable_console_colours(...) needs to be called before quill::start(). That can "
      "cause a race condition on the backend worker thread. Catch the exception to "
      "avoid this error but the call to quill::enable_console_colours(...) will have no "
      "effect."});
  }

  detail::LogManagerSingleton::instance().log_manager().logger_collection().enable_console_colours();
}

/***/
void flush() { detail::LogManagerSingleton::instance().log_manager().flush(); }

/***/
#if !defined(QUILL_NO_EXCEPTIONS)
void set_backend_worker_error_handler(backend_worker_error_handler_t backend_worker_error_handler)
{
  if (detail::LogManagerSingleton::instance().log_manager().backend_worker_is_running())
  {
    QUILL_THROW(QuillError{
      "quill::set_backend_worker_error_handler(...) needs to be called before quill::start(). That "
      "can cause a race condition on the backend worker thread. Catch the exception to "
      "avoid this error but the call to quill::set_backend_worker_error_handler(...) will have no "
      "effect."});
  }

  detail::LogManagerSingleton::instance().log_manager().set_backend_worker_error_handler(
    std::move(backend_worker_error_handler));
}
#endif

// ** Config ** //
namespace config
{
/***/
void set_backend_thread_cpu_affinity(uint16_t cpu)
{
  if (detail::LogManagerSingleton::instance().log_manager().backend_worker_is_running())
  {
    QUILL_THROW(QuillError{
      "quill::set_backend_thread_cpu_affinity(...) needs to be called before quill::start(). That "
      "can cause a race condition on the backend worker thread. Catch the exception to "
      "avoid this error but the call to quill::set_backend_thread_cpu_affinity(...) will have no "
      "effect."});
  }

  detail::LogManagerSingleton::instance().log_manager().config().set_backend_thread_cpu_affinity(cpu);
}

/***/
void set_backend_thread_name(std::string const& name)
{
  if (detail::LogManagerSingleton::instance().log_manager().backend_worker_is_running())
  {
    QUILL_THROW(QuillError{
      "quill::set_backend_thread_name(...) needs to be called before quill::start(). That can "
      "cause a race condition on the backend worker thread. Catch the exception to "
      "avoid this error but the call to quill::set_backend_thread_name(...) will have no "
      "effect."});
  }

  detail::LogManagerSingleton::instance().log_manager().config().set_backend_thread_name(name);
}

/***/
void set_backend_thread_sleep_duration(std::chrono::nanoseconds sleep_duration)
{
  if (detail::LogManagerSingleton::instance().log_manager().backend_worker_is_running())
  {
    QUILL_THROW(QuillError{
      "quill::set_backend_thread_sleep_duration(...) needs to be called before quill::start(). "
      "That can cause a race condition on the backend worker thread. Catch the exception to "
      "avoid this error but the call to quill::set_backend_thread_sleep_duration(...) will have no "
      "effect."});
  }

  detail::LogManagerSingleton::instance().log_manager().config().set_backend_thread_sleep_duration(sleep_duration);
}

/***/
QUILL_ATTRIBUTE_COLD void set_backend_thread_max_transit_events(size_t max_transit_events)
{
  if (detail::LogManagerSingleton::instance().log_manager().backend_worker_is_running())
  {
    QUILL_THROW(QuillError{
      "quill::set_backend_worker_max_transit_events(...) needs to be called before quill::start(). "
      "That can cause a race condition on the backend worker thread. Catch the exception to "
      "avoid this error but the call to quill::set_backend_worker_max_transit_events(...) will "
      "have no "
      "effect."});
  }

  detail::LogManagerSingleton::instance().log_manager().config().set_backend_thread_max_transit_events(max_transit_events);
}
} // namespace config

} // namespace quill