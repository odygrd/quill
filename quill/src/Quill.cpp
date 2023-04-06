#include "quill/Quill.h"
#include "quill/QuillError.h"
#include "quill/detail/HandlerCollection.h"         // for HandlerCollection
#include "quill/detail/LogManager.h"                // for LogManagerSingleton
#include "quill/detail/LoggerCollection.h"          // for LoggerCollection
#include "quill/detail/ThreadContext.h"             // for ThreadContext, Thr...
#include "quill/detail/ThreadContextCollection.h"   // for ThreadContextColle...
#include "quill/handlers/ConsoleHandler.h"          // for ConsoleHandler
#include "quill/handlers/FileHandler.h"             // for FileHandler, Filenam...
#include "quill/handlers/JsonFileHandler.h"         // for JsonFileHandler
#include "quill/handlers/RotatingFileHandler.h"     // for RotatingFileHandler
#include "quill/handlers/StreamHandler.h"           // for StreamHandler
#include "quill/handlers/TimeRotatingFileHandler.h" // for TimeRotatingFileHandler
#include <cassert>
#include <utility> // for move

namespace quill
{

Logger* _g_root_logger = nullptr;

/***/
QUILL_ATTRIBUTE_COLD void configure(Config& config)
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
std::shared_ptr<Handler> file_handler(fs::path const& filename, std::string const& mode, /* = std::string{} */
                                      FilenameAppend append_to_filename /* = FilenameAppend::None */,
                                      FileEventNotifier file_event_notifier /* = FileEventNotifier{} */,
                                      bool do_fsync /* = false */)
{
  return create_handler<FileHandler>(filename.string(), mode, append_to_filename,
                                     std::move(file_event_notifier), do_fsync);
}

/***/
std::shared_ptr<Handler> time_rotating_file_handler(
  fs::path const& base_filename, std::string const& mode /* = std::string{"a"} */,
  FilenameAppend append_to_filename /* = FilenameAppend::None */,
  std::string const& when /* = std::string{"H"} */, uint32_t interval /* = 1 */, uint32_t backup_count /* = 0 */,
  Timezone timezone /* = Timezone::LocalTime */, std::string const& at_time /* = std::string{} */,
  FileEventNotifier file_event_notifier /* = FileEventNotifier{} */, bool do_fsync /* = false */)
{
  return create_handler<TimeRotatingFileHandler>(base_filename.string(), mode, append_to_filename,
                                                 when, interval, backup_count, timezone, at_time,
                                                 std::move(file_event_notifier), do_fsync);
}

/***/
std::shared_ptr<Handler> rotating_file_handler(
  fs::path const& base_filename, std::string const& mode /* = std::string {"a"} */,
  FilenameAppend append_to_filename /* = FilenameAppend::None */, size_t max_bytes /* = 0 */,
  uint32_t backup_count /* = 0 */, bool overwrite_oldest_files /* = true */, bool clean_old_files /* = false */,
  FileEventNotifier file_event_notifier /* = FileEventNotifier{} */, bool do_fsync /* = false */)
{
  return create_handler<RotatingFileHandler>(base_filename.string(), mode, append_to_filename,
                                             max_bytes, backup_count, overwrite_oldest_files,
                                             clean_old_files, std::move(file_event_notifier), do_fsync);
}

/***/
std::shared_ptr<Handler> json_file_handler(fs::path const& filename, std::string const& mode,
                                           FilenameAppend append_to_filename,
                                           FileEventNotifier file_event_notifier /* = FileEventNotifier{} */,
                                           bool do_fsync /* = false */)
{
  return create_handler<JsonFileHandler>(filename.string(), mode, append_to_filename,
                                         std::move(file_event_notifier), do_fsync);
}

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
  detail::LogManagerSingleton::instance().log_manager().logger_collection().remove_logger(logger);
}

/***/
void flush() { detail::LogManagerSingleton::instance().log_manager().flush(); }

/***/
void wake_up_logging_thread()
{
  detail::LogManagerSingleton::instance().log_manager().wake_up_backend_worker();
}

} // namespace quill
