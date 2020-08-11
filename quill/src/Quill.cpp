#include "quill/Quill.h"
#include "quill/detail/Config.h"                  // for Config
#include "quill/detail/HandlerCollection.h"       // for HandlerCollection
#include "quill/detail/LogManagerSingleton.h"     // for LogManagerSingleton
#include "quill/detail/LoggerCollection.h"        // for LoggerCollection
#include "quill/detail/ThreadContext.h"           // for ThreadContext, Thr...
#include "quill/detail/ThreadContextCollection.h" // for ThreadContextColle...
#include "quill/handlers/StreamHandler.h"         // for StreamHandler
#include <utility>                                // for move

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
Handler* stdout_handler(std::string const& stdout_handler_name /* = "stdout" */)
{
  return detail::LogManagerSingleton::instance().log_manager().handler_collection().stdout_streamhandler(
    stdout_handler_name);
}

/***/
Handler* stderr_handler(std::string const& stderr_handler_name /* = "stderr" */)
{
  return detail::LogManagerSingleton::instance().log_manager().handler_collection().stderr_streamhandler(
    stderr_handler_name);
}

/***/
Handler* file_handler(filename_t const& filename, std::string const& mode, /* = std::string{} */
                      FilenameAppend append_to_filename /* = FilenameAppend::None */)
{
  return detail::LogManagerSingleton::instance().log_manager().handler_collection().file_handler(
    filename, mode, append_to_filename);
}

/***/
Handler* time_rotating_file_handler(filename_t const& base_filename,
                                    std::string const& mode /* = std::string{"a"} */,
                                    std::string const& when /* = std::string{"H"} */,
                                    uint32_t interval /* = 1 */, uint32_t backup_count /* = 0 */,
                                    Timezone timezone /* = Timezone::LocalTime */,
                                    std::string const& at_time /* = std::string{} */)
{
  return detail::LogManagerSingleton::instance().log_manager().handler_collection().time_rotating_file_handler(
    base_filename, mode, when, interval, backup_count, timezone, at_time);
}

/***/
Handler* rotating_file_handler(filename_t const& base_filename,
                               std::string const& mode /* = std::string {"a"} */,
                               size_t max_bytes /* = 0 */,
                               uint32_t backup_count /* = 0 */)
{
  return detail::LogManagerSingleton::instance().log_manager().handler_collection().rotating_file_handler(
    base_filename, mode, max_bytes, backup_count);
}

#if defined(_WIN32)
/***/
Handler* file_handler(std::string const& filename, std::string const& mode /* = std::string{} */,
                      FilenameAppend append_to_filename /* = FilenameAppend::None */)
{
  return file_handler(detail::s2ws(filename), mode, append_to_filename);
}

/***/
Handler* time_rotating_file_handler(std::string const& base_filename,
                                    std::string const& mode /* = std::string{"a"} */,
                                    std::string const& when /* = std::string{"H"} */,
                                    uint32_t interval /* = 1 */, uint32_t backup_count /* = 0 */,
                                    Timezone timezone /* = Timezone::LocalTime */,
                                    std::string const& at_time /* = std::string{} */)
{
  return detail::LogManagerSingleton::instance().log_manager().handler_collection().time_rotating_file_handler(
    detail::s2ws(base_filename), mode, when, interval, backup_count, timezone, at_time);
}

/***/
Handler* rotating_file_handler(std::string const& base_filename,
                               std::string const& mode /* = std::string {"a"} */,
                               size_t max_bytes /* = 0 */, uint32_t backup_count /* = 0 */)
{
  return rotating_file_handler(detail::s2ws(base_filename), mode, max_bytes, backup_count);
}
#endif

/***/
Logger* get_logger(char const* logger_name /* = nullptr */)
{
  return detail::LogManagerSingleton::instance().log_manager().logger_collection().get_logger(logger_name);
}

/***/
Logger* create_logger(char const* logger_name)
{
  return detail::LogManagerSingleton::instance().log_manager().logger_collection().create_logger(logger_name);
}

/***/
Logger* create_logger(char const* logger_name, Handler* handler)
{
  return detail::LogManagerSingleton::instance().log_manager().logger_collection().create_logger(
    logger_name, handler);
}

/***/
Logger* create_logger(char const* logger_name, std::initializer_list<Handler*> handlers)
{
  return detail::LogManagerSingleton::instance().log_manager().logger_collection().create_logger(
    logger_name, handlers);
}

/***/
void set_default_logger_handler(Handler* handler)
{
  detail::LogManagerSingleton::instance().log_manager().logger_collection().set_default_logger_handler(handler);
}

/***/
void set_default_logger_handler(std::initializer_list<Handler*> handlers)
{
  detail::LogManagerSingleton::instance().log_manager().logger_collection().set_default_logger_handler(handlers);
}

/***/
void flush() { detail::LogManagerSingleton::instance().log_manager().flush(); }

/***/
#if !defined(QUILL_NO_EXCEPTIONS)
void set_backend_worker_error_handler(backend_worker_error_handler_t backend_worker_error_handler)
{
  detail::LogManagerSingleton::instance().log_manager().set_backend_worker_error_handler(
    std::move(backend_worker_error_handler));
}
#endif

// ** Config ** //
namespace config
{
/***/
void set_backend_thread_cpu_affinity(uint16_t cpu) noexcept
{
  detail::LogManagerSingleton::instance().log_manager().config().set_backend_thread_cpu_affinity(cpu);
}

/***/
void set_backend_thread_name(std::string const& name) noexcept
{
  detail::LogManagerSingleton::instance().log_manager().config().set_backend_thread_name(name);
}

/***/
void set_backend_thread_sleep_duration(std::chrono::nanoseconds sleep_duration) noexcept
{
  detail::LogManagerSingleton::instance().log_manager().config().set_backend_thread_sleep_duration(sleep_duration);
}

} // namespace config

} // namespace quill