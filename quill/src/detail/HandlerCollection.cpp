#include "quill/detail/HandlerCollection.h"
#include "quill/QuillError.h"                       // for QuillError
#include "quill/detail/misc/Utilities.h"            // for s2ws
#include "quill/handlers/ConsoleHandler.h"          // for ConsoleHandler
#include "quill/handlers/FileHandler.h"             // for FileHandler, Filenam...
#include "quill/handlers/RotatingFileHandler.h"     // for RotatingFileHandler
#include "quill/handlers/TimeRotatingFileHandler.h" // for TimeRotatingFileHandler
#include <algorithm>                                // for find_if
#include <cstdio>                                   // for stdout,stderr
#include <mutex>                                    // for lock_guard
#include <utility>                                  // for pair

namespace quill
{
namespace detail
{
/***/
StreamHandler* HandlerCollection::stdout_console_handler(std::string const& stdout_handler_name /* = std::string{"stdout"} */,
                                                         ConsoleColours const& console_colours /* = ConsoleColours{} */)
{
#if defined(_WIN32)
  return _create_console_handler(s2ws(stdout_handler_name), stdout, console_colours);
#else
  return _create_console_handler(stdout_handler_name, stdout, console_colours);
#endif
}

/***/
StreamHandler* HandlerCollection::stderr_console_handler(std::string const& stderr_handler_name /* = std::string{"stderr"} */)
{
  // we just pass an empty ConsoleColours class, as we don't use colours for stderr.
#if defined(_WIN32)
  return _create_console_handler(s2ws(stderr_handler_name), stderr, quill::ConsoleColours{});
#else
  return _create_console_handler(stderr_handler_name, stderr, quill::ConsoleColours{});
#endif
}

/***/
StreamHandler* HandlerCollection::file_handler(filename_t const& filename,
                                               std::string const& mode /* = std::string{"a"} */,
                                               FilenameAppend append_to_filename /* = FilenameAppend::None */)
{
  // Protect shared access
  std::lock_guard<Spinlock> const lock{_spinlock};

  // Try to insert it unless we failed it means we already had it
  auto const search = _file_handler_collection.find(filename);

  // First search if we have it and don't call make_unique yet as this will call fopen
  if (search != _file_handler_collection.cend())
  {
    return (*search).second.get();
  }

  // if first time add it
  auto emplace_result = _file_handler_collection.emplace(
    filename, std::make_unique<FileHandler>(filename.data(), mode.data(), append_to_filename));

  return (*emplace_result.first).second.get();
}

/***/
StreamHandler* HandlerCollection::time_rotating_file_handler(filename_t const& base_filename,
                                                             std::string const& mode, std::string const& when,
                                                             uint32_t interval, uint32_t backup_count,
                                                             Timezone timezone, std::string const& at_time)
{
  // Protect shared access
  std::lock_guard<Spinlock> const lock{_spinlock};

  // Try to insert it unless we failed it means we already had it
  auto const search = _file_handler_collection.find(base_filename);

  // First search if we have it and don't call make_unique yet as this will call fopen
  if (search != _file_handler_collection.cend())
  {
    return (*search).second.get();
  }

  // if first time add it
  auto emplace_result = _file_handler_collection.emplace(
    base_filename,
    std::make_unique<TimeRotatingFileHandler>(base_filename.data(), mode, when, interval,
                                              backup_count, timezone, at_time));

  return (*emplace_result.first).second.get();
}

/***/
StreamHandler* HandlerCollection::rotating_file_handler(filename_t const& base_filename, std::string const& mode,
                                                        size_t max_file_size, uint32_t backup_count)
{
  // Protect shared access
  std::lock_guard<Spinlock> const lock{_spinlock};

  // Try to insert it unless we failed it means we already had it
  auto const search = _file_handler_collection.find(base_filename);

  // First search if we have it and don't call make_unique yet as this will call fopen
  if (search != _file_handler_collection.cend())
  {
    return (*search).second.get();
  }

  // if first time add it
  auto emplace_result = _file_handler_collection.emplace(
    base_filename, std::make_unique<RotatingFileHandler>(base_filename.data(), mode, max_file_size, backup_count));

  return (*emplace_result.first).second.get();
}

/***/
void HandlerCollection::subscribe_handler(Handler* handler_to_insert)
{
  // Protect shared access
  std::lock_guard<Spinlock> const lock{_spinlock};

  // Check if we already have this object
  auto const search = std::find_if(
    _active_handlers_collection.cbegin(), _active_handlers_collection.cend(),
    [&handler_to_insert](Handler const* handler_elem) { return handler_elem == handler_to_insert; });

  if (search == _active_handlers_collection.cend())
  {
    // we don't have this object so add it
    _active_handlers_collection.push_back(handler_to_insert);
  }
}

/***/
std::vector<Handler*> HandlerCollection::active_handlers() const
{
  std::vector<Handler*> subscribed_handlers_collection;

  // Protect shared access, we just use a lock here since this function is not used when logging
  // messages but only in special cases e.g. flushing
  std::lock_guard<Spinlock> const lock{_spinlock};
  subscribed_handlers_collection = _active_handlers_collection;

  return subscribed_handlers_collection;
}

/***/
StreamHandler* HandlerCollection::_create_console_handler(filename_t const& stream, FILE* file,
                                                          ConsoleColours const& console_colours)
{
  // Protect shared access
  std::lock_guard<Spinlock> const lock{_spinlock};

  // Try to insert it unless we failed it means we already had it
  auto const search = _file_handler_collection.find(stream);

  // First search if we have it and don't call make_unique yet as this will call fopen
  if (search != _file_handler_collection.cend())
  {
    // if the handler with that name was already created check that the user didn't try to create it
    // again passing a different file
    if (file == stdout && (search->second->stream_handler_type() != StreamHandler::StreamHandlerType::Stdout))
    {
      QUILL_THROW(QuillError(
        "Trying to insert an stdout handler again, but the handler already exists as a different "
        "file. Use an unique stream_handler name"));
    }
    else if (file == stderr && (search->second->stream_handler_type() != StreamHandler::StreamHandlerType::Stderr))
    {
      QUILL_THROW(QuillError(
        "Trying to insert an stderr handler again, but the handler already exists as a different "
        "file. Use an unique stream_handler name"));
    }
    else if (search->second->stream_handler_type() == StreamHandler::StreamHandlerType::File)
    {
      QUILL_THROW(QuillError(
        "Trying to insert an stdout/stderr handler, but the handler already exists. Use an unique "
        "stream_handler name"));
    }

    return (*search).second.get();
  }

  // if first time add it
  auto emplace_result = _file_handler_collection.emplace(
    stream, std::make_unique<ConsoleHandler>(stream, file, console_colours));

  return (*emplace_result.first).second.get();
}
} // namespace detail
} // namespace quill