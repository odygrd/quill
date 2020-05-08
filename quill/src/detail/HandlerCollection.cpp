#include "quill/detail/HandlerCollection.h"
#include "quill/handlers/DailyFileHandler.h"    // for DailyFileHandler
#include "quill/handlers/FileHandler.h"         // for FileHandler, Filenam...
#include "quill/handlers/RotatingFileHandler.h" // for RotatingFileHandler
#include <algorithm>                            // for find_if
#include <mutex>                                // for lock_guard
#include <utility>                              // for pair

namespace quill
{
namespace detail
{
/***/
StreamHandler* HandlerCollection::stdout_streamhandler()
{
#if defined(_WIN32)
  return _create_streamhandler(std::wstring{L"stdout"});
#else
  return _create_streamhandler(std::string{"stdout"});
#endif
}

/***/
StreamHandler* HandlerCollection::stderr_streamhandler()
{
#if defined(_WIN32)
  return _create_streamhandler(std::wstring{L"stderr"});
#else
  return _create_streamhandler(std::string{"stderr"});
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
StreamHandler* HandlerCollection::daily_file_handler(filename_t const& base_filename,
                                                     std::chrono::hours rotation_hour,
                                                     std::chrono::minutes rotation_minute)
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
    base_filename, std::make_unique<DailyFileHandler>(base_filename.data(), rotation_hour, rotation_minute));

  return (*emplace_result.first).second.get();
}

/***/
StreamHandler* HandlerCollection::rotating_file_handler(filename_t const& base_filename, size_t max_file_size)
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
    base_filename, std::make_unique<RotatingFileHandler>(base_filename.data(), max_file_size));

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
    [&handler_to_insert](Handler* handler_elem) { return handler_elem == handler_to_insert; });

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
StreamHandler* HandlerCollection::_create_streamhandler(filename_t const& stream)
{
  // Protect shared access
  std::lock_guard<Spinlock> const lock{_spinlock};

  // Try to insert it unless we failed it means we already had it
  auto const search = _file_handler_collection.find(stream);

  // First search if we have it and don't call make_unique yet as this will call fopen
  if (search != _file_handler_collection.cend())
  {
    return (*search).second.get();
  }

  // if first time add it
  auto emplace_result = _file_handler_collection.emplace(stream, std::make_unique<StreamHandler>(stream));

  return (*emplace_result.first).second.get();
}
} // namespace detail
} // namespace quill