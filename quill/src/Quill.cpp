#include "quill/Quill.h"
#include "quill/QuillError.h"
#include "quill/detail/HandlerCollection.h"       // for HandlerCollection
#include "quill/detail/LogManager.h"              // for LogManagerSingleton
#include "quill/detail/LoggerCollection.h"        // for LoggerCollection
#include "quill/detail/ThreadContext.h"           // for ThreadContext, Thr...
#include "quill/detail/ThreadContextCollection.h" // for ThreadContextColle...
#include "quill/handlers/ConsoleHandler.h"        // for ConsoleHandler
#include "quill/handlers/NullHandler.h"           // for NullHandler
#include "quill/handlers/StreamHandler.h"         // for StreamHandler
#include <cassert>
#include <utility> // for move

namespace quill
{

Logger* _g_root_logger = nullptr;

/***/
QUILL_ATTRIBUTE_COLD void configure(Config const& config)
{
  if (detail::LogManagerSingleton::instance().log_manager().backend_worker_is_running())
  {
    QUILL_THROW(QuillError{"quill::configure(...) needs to be called before quill::start()"});
  }

  return detail::LogManagerSingleton::instance().log_manager().configure(config);
}

/***/
std::shared_ptr<Handler> stdout_handler(std::string const& stdout_handler_name /* = "stdout" */,
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
std::shared_ptr<Handler> stderr_handler(std::string const& stderr_handler_name /* = "stderr" */)
{
  return detail::LogManagerSingleton::instance().log_manager().handler_collection().stderr_console_handler(
    stderr_handler_name);
}

/***/
std::shared_ptr<Handler> get_handler(std::string const& handler_name)
{
  return detail::LogManagerSingleton::instance().log_manager().handler_collection().get_handler(handler_name);
}

/***/
std::shared_ptr<Handler> file_handler(fs::path const& filename, FileHandlerConfig const& config, /* = FileHandlerConfig{} */
                                      FileEventNotifier file_event_notifier /* = FileEventNotifier{} */)
{
  return create_handler<FileHandler>(filename.string(), config, std::move(file_event_notifier));
}

/***/
std::shared_ptr<Handler> rotating_file_handler(fs::path const& base_filename,
                                               RotatingFileHandlerConfig const& config, /* = RotatingFileHandlerConfig{} */
                                               FileEventNotifier file_event_notifier /* = FileEventNotifier{} */)
{
  return create_handler<RotatingFileHandler>(base_filename.string(), config, std::move(file_event_notifier));
}

/***/
std::shared_ptr<Handler> json_file_handler(fs::path const& filename, JsonFileHandlerConfig const& config, /* = JsonFileHandlerConfig{} */
                                           FileEventNotifier file_event_notifier /* = FileEventNotifier{} */)
{
  return create_handler<JsonFileHandler>(filename.string(), config, std::move(file_event_notifier));
}

/***/
std::shared_ptr<Handler> null_handler() { return create_handler<NullHandler>("nullhandler"); }

/***/
Logger* get_logger(char const* logger_name /* = nullptr */)
{
  if (!logger_name)
  {
    if (!_g_root_logger)
    {
      // this means the LoggerCollection has not been constructed yet, someone called this
      // before quill::start();
      return detail::LogManagerSingleton::instance().log_manager().logger_collection().get_logger(logger_name);
    }

    return _g_root_logger;
  }

  return detail::LogManagerSingleton::instance().log_manager().logger_collection().get_logger(logger_name);
}

/***/
Logger* get_root_logger() noexcept
{
  assert(_g_root_logger &&
         "_g_root_logger is nullptr, this function must be called after quill::start()");
  return _g_root_logger;
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
Logger* create_logger(std::string const& logger_name, std::shared_ptr<Handler>&& handler,
                      std::optional<TimestampClockType> timestamp_clock_type /* = std::nullopt */,
                      std::optional<TimestampClock*> timestamp_clock /* = std::nullopt */)
{
  return detail::LogManagerSingleton::instance().log_manager().create_logger(
    logger_name, std::move(handler), timestamp_clock_type, timestamp_clock);
}

/***/
Logger* create_logger(std::string const& logger_name, std::initializer_list<std::shared_ptr<Handler>> handlers,
                      std::optional<TimestampClockType> timestamp_clock_type /* = std::nullopt */,
                      std::optional<TimestampClock*> timestamp_clock /* = std::nullopt */)
{
  return detail::LogManagerSingleton::instance().log_manager().create_logger(
    logger_name, handlers, timestamp_clock_type, timestamp_clock);
}

/***/
Logger* create_logger(std::string const& logger_name, std::vector<std::shared_ptr<Handler>>&& handlers,
                      std::optional<TimestampClockType> timestamp_clock_type /* = std::nullopt */,
                      std::optional<TimestampClock*> timestamp_clock /* = std::nullopt */)
{
  return detail::LogManagerSingleton::instance().log_manager().create_logger(
    logger_name, std::move(handlers), timestamp_clock_type, timestamp_clock);
}

/***/
void remove_logger(Logger* logger)
{
  Logger* default_logger = get_logger(nullptr);

  if (logger == default_logger)
  {
    // we do not allow removing the default logger as it is also used by the flush() function
    return;
  }

  detail::LogManagerSingleton::instance().log_manager().logger_collection().remove_logger(logger);
}

/***/
void wake_up_logging_thread()
{
  detail::LogManagerSingleton::instance().log_manager().wake_up_backend_worker();
}

} // namespace quill
