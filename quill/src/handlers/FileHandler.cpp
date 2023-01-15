#include "quill/handlers/FileHandler.h"
#include "quill/detail/misc/FileUtilities.h" // for append_date_to_filename
#include <cstdio>                            // for fclose

namespace quill
{
/***/
FileHandler::FileHandler(fs::path const& filename, std::string const& mode,
                         FilenameAppend append_to_filename, FileEventNotifier file_event_notifier)
  : StreamHandler(filename), _file_event_notifier(std::move(file_event_notifier))
{
  if (append_to_filename == FilenameAppend::None)
  {
    _current_filename = filename;
  }
  else if (append_to_filename == FilenameAppend::Date)
  {
    _current_filename = detail::append_date_to_filename(_filename);
  }
  else if (append_to_filename == FilenameAppend::DateTime)
  {
    _current_filename = detail::append_date_to_filename(_filename, std::chrono::system_clock::now(), true);
  }

  open_file(_current_filename, mode);
}

/***/
FileHandler::FileHandler(fs::path const& filename,
                         FileEventNotifier file_event_notifier /* = FileEventNotifier{} */)
  : StreamHandler(filename), _file_event_notifier(std::move(file_event_notifier))
{
}

/***/
void FileHandler::open_file(fs::path const& filename, std::string const& mode)
{
  if (_file_event_notifier.before_open)
  {
    _file_event_notifier.before_open(_current_filename);
  }

  // _file is the base file*
  _file = detail::open_file(_current_filename, mode);

  assert(_file && "open_file always returns a valid pointer");

  if (_file_event_notifier.after_open)
  {
    _file_event_notifier.after_open(_current_filename, _file);
  }
}

/***/
void FileHandler::close_file()
{
  if (_file)
  {
    if (_file_event_notifier.before_close)
    {
      _file_event_notifier.before_close(_current_filename, _file);
    }

    fclose(_file);
    _file = nullptr;

    if (_file_event_notifier.after_close)
    {
      _file_event_notifier.after_close(_current_filename);
    }
  }
}

/***/
FileHandler::~FileHandler() { close_file(); }
} // namespace quill