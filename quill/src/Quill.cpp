#include "quill/Quill.h"

#include "quill/detail/LogManagerSingleton.h"

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
Handler* stdout_handler()
{
  return detail::LogManagerSingleton::instance().log_manager().handler_collection().stdout_streamhandler();
}

/***/
Handler* stderr_handler()
{
  return detail::LogManagerSingleton::instance().log_manager().handler_collection().stderr_streamhandler();
}

/***/
Handler* file_handler(filename_t const& filename,
                      std::string const& mode, /* = std::string{} */
                      FilenameAppend append_to_filename /* = FilenameAppend::None */)
{
  return detail::LogManagerSingleton::instance().log_manager().handler_collection().file_handler(
    filename, mode, append_to_filename);
}

/***/
Handler* daily_file_handler(filename_t const& base_filename, std::chrono::hours rotation_hour, std::chrono::minutes rotation_minute)
{
  return detail::LogManagerSingleton::instance().log_manager().handler_collection().daily_file_handler(
    base_filename, rotation_hour, rotation_minute);
}

/***/
Handler* rotating_file_handler(filename_t const& base_filename, size_t max_bytes)
{
  return detail::LogManagerSingleton::instance().log_manager().handler_collection().rotating_file_handler(
    base_filename, max_bytes);
}

#if defined(_WIN32)
/***/
Handler* file_handler(std::string const& filename,
                      std::string const& mode /* = std::string{} */,
                      FilenameAppend append_to_filename /* = FilenameAppend::None */)
{
  return file_handler(detail::s2ws(filename), mode, append_to_filename);
}

/***/
Handler* daily_file_handler(std::string const& base_filename,
                            std::chrono::hours rotation_hour,
                            std::chrono::minutes rotation_minute)
{
  return daily_file_handler(detail::s2ws(base_filename), rotation_hour, rotation_minute);
}

/***/
Handler* rotating_file_handler(std::string const& base_filename, size_t max_bytes)
{
  return rotating_file_handler(detail::s2ws(base_filename), max_bytes);
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

/***/
void set_initial_queue_capacity(size_t initial_capacity) noexcept
{
  detail::LogManagerSingleton::instance().log_manager().config().set_initial_queue_capacity(initial_capacity);
}

} // namespace config

} // namespace quill