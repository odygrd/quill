#include "quill/handlers/FileHandler.h"
#include "quill/Fmt.h"
#include "quill/detail/misc/FileUtilities.h" // for append_date_to_filename
#include "quill/detail/misc/Os.h"
#include <cstdio> // for fclose

namespace
{
QUILL_NODISCARD quill::fs::path get_appended_filename(quill::fs::path const& filename,
                                                      quill::FilenameAppend append_to_filename,
                                                      quill::Timezone timezone)
{
  if ((append_to_filename == quill::FilenameAppend::None) || (filename == "/dev/null"))
  {
    return filename;
  }
  else if (append_to_filename == quill::FilenameAppend::StartDate)
  {
    return quill::detail::append_date_time_to_filename(filename, false, timezone);
  }
  else if (append_to_filename == quill::FilenameAppend::StartDateTime)
  {
    return quill::detail::append_date_time_to_filename(filename, true, timezone);
  }

  return quill::fs::path{};
}
} // namespace

namespace quill
{
/***/
void FileHandlerConfig::set_append_to_filename(FilenameAppend value)
{
  _append_to_filename = value;
}

/***/
void FileHandlerConfig::set_timezone(Timezone timezone) { _timezone_value = timezone; }

/***/
void FileHandlerConfig::set_do_fsync(bool value) { _do_fsync = value; }

/***/
void FileHandlerConfig::set_open_mode(char open_mode) { _open_mode = open_mode; }

/***/
void FileHandlerConfig::set_pattern(std::string const& log_pattern,
                                    std::string const& time_format /* = std::string{"%H:%M:%S.%Qns"} */)
{
  _log_pattern = log_pattern;
  _time_format = time_format;
}

/***/
FileHandler::FileHandler(fs::path const& filename, FileHandlerConfig config,
                         FileEventNotifier file_event_notifier, bool do_fopen /* = true */)
  : StreamHandler(get_appended_filename(filename, config.append_to_filename(), config.timezone()),
                  nullptr, std::move(file_event_notifier)),
    _config(config)
{
  if (!_config.log_pattern().empty())
  {
    set_pattern(_config.log_pattern(), _config.time_format());
  }

  if (do_fopen)
  {
    open_file(_filename, _config.open_mode());
  }
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

  if (_config.do_fsync())
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
