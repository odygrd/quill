#include "quill/handlers/StreamHandler.h"
#include "quill/detail/misc/FileUtilities.h" // for fwrite_fully
#include <string>                            // for allocator, operator==
#include <utility>                           // for move

namespace quill
{
/***/
StreamHandler::StreamHandler(fs::path stream, FILE* file /* = nullptr */,
                             FileEventNotifier file_event_notifier /* = FileEventNotifier{} */)
  : _filename(std::move(stream)), _file(file), _file_event_notifier(std::move(file_event_notifier))
{
  // reserve stdout and stderr as filenames
  if (_filename == std::string{"stdout"})
  {
    _file = stdout;
  }
  else if (_filename == std::string{"stderr"})
  {
    _file = stderr;
  }
  else if (_filename == std::string{"/dev/null"})
  {
    _is_null = true;
  }
  else
  {
    // first attempt to create any non-existing directories
    std::error_code ec;
    fs::path parent_path;

    if (!_filename.parent_path().empty())
    {
      parent_path = _filename.parent_path();
      fs::create_directories(parent_path, ec);
      if (ec)
      {
        // use .string() to also support experimental fs
        QUILL_THROW(QuillError{fmtquill::format("cannot create directories for {}, error: {}",
                                                parent_path.string(), ec.message())});
      }
    }
    else
    {
      parent_path = fs::current_path();
    }

    // convert the parent path to an absolute path
    fs::path const canonical_path = fs::canonical(parent_path, ec);

    if (ec)
    {
      // use .string() to also support experimental fs
      QUILL_THROW(QuillError{fmtquill::format("cannot make canonical path for {}, error: {}",
                                              parent_path.string(), ec.message())});
    }

    // finally replace the given filename's parent_path with the equivalent canonical path
    _filename = canonical_path / _filename.filename();
  }
}

/***/
void StreamHandler::write(fmt_buffer_t const& formatted_log_message, TransitEvent const&)
{
  if (QUILL_UNLIKELY(!_file))
  {
    // FileHandler::flush() tries to re-open a deleted file and if it fails _file can be null
    return;
  }

  if (_file_event_notifier.before_write)
  {
    std::string const modified_message = _file_event_notifier.before_write(
      std::string_view{formatted_log_message.data(), formatted_log_message.size()});

    detail::fwrite_fully(modified_message.data(), sizeof(char), modified_message.size(), _file);
  }
  else
  {
    detail::fwrite_fully(formatted_log_message.data(), sizeof(char), formatted_log_message.size(), _file);
  }

  _write_occurred = true;
}

/***/
void StreamHandler::flush()
{
  if (!_write_occurred || !_file)
  {
    return;
  }

  _write_occurred = false;
  fflush(_file);
}

/***/
fs::path const& StreamHandler::filename() const noexcept { return _filename; }

/***/
StreamHandler::StreamHandlerType StreamHandler::stream_handler_type() const noexcept
{
  if (_file == stdout)
  {
    return StreamHandlerType::Stdout;
  }

  if (_file == stderr)
  {
    return StreamHandlerType::Stderr;
  }

  return StreamHandlerType::File;
}

/***/
bool StreamHandler::is_null() const noexcept { return _is_null; }
} // namespace quill
