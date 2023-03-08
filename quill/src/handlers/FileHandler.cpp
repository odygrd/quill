#include "quill/handlers/FileHandler.h"
#include "quill/detail/misc/FileUtilities.h" // for append_date_to_filename
#include "quill/detail/misc/Os.h"
#include <cstdio> // for fclose

namespace
{
QUILL_NODISCARD quill::fs::path get_filename(quill::FilenameAppend append_to_filename, quill::fs::path const& filename)
{
  if (append_to_filename == quill::FilenameAppend::None)
  {
    return filename;
  }
  else if (append_to_filename == quill::FilenameAppend::Date)
  {
    return quill::detail::append_date_to_filename(filename);
  }
  else if (append_to_filename == quill::FilenameAppend::DateTime)
  {
    return quill::detail::append_date_to_filename(filename, std::chrono::system_clock::now(), true);
  }

  return quill::fs::path{};
}
} // namespace

namespace quill
{
/***/
FileHandler::FileHandler(fs::path const& filename, std::string const& mode, FilenameAppend append_to_filename,
                         FileEventNotifier file_event_notifier, bool do_fsync)
  : StreamHandler(get_filename(append_to_filename, filename), nullptr, std::move(file_event_notifier)),
    _fsync(do_fsync)
{
  open_file(_filename, mode);
}

/***/
FileHandler::FileHandler(fs::path const& filename, FilenameAppend append_to_filename,
                         FileEventNotifier file_event_notifier, bool do_fsync)
  : StreamHandler(get_filename(append_to_filename, filename), nullptr, std::move(file_event_notifier)), _fsync(do_fsync)
{
}

/***/
void FileHandler::open_file(fs::path const& filename, std::string const& mode)
{
  if (_file_event_notifier.before_open)
  {
    _file_event_notifier.before_open(filename);
  }

  // _file is the base file*
  _file = detail::open_file(filename, mode);

  assert(_file && "open_file always returns a valid pointer");

  if (_file_event_notifier.after_open)
  {
    _file_event_notifier.after_open(filename, _file);
  }
}

/***/
void FileHandler::close_file()
{
  if (_file)
  {
    if (_file_event_notifier.before_close)
    {
      _file_event_notifier.before_close(_filename, _file);
    }

    fclose(_file);
    _file = nullptr;

    if (_file_event_notifier.after_close)
    {
      _file_event_notifier.after_close(_filename);
    }
  }
}

/***/
FileHandler::~FileHandler() { close_file(); }

/***/
void FileHandler::flush() noexcept
{
  StreamHandler::flush();

  if (_fsync)
  {
    detail::fsync(_file);
  }
  
  if (!fs::exists(_filename))
  {
    // after flushing the file we can check if the file still exists. If not we reopen it.
    // This can happen if a user deletes a file while the application is running
    close_file();

    // now reopen the file for writing again, it will be a new file
    open_file(_filename, "w");
  }
}

} // namespace quill
