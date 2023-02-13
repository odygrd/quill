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
        QUILL_THROW(QuillError{
          fmt::format("cannot create directories for {}, error: {}", parent_path, ec.message())});
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
      QUILL_THROW(QuillError{
        fmt::format("cannot make canonical path for {}, error: {}", parent_path, ec.message())});
    }

    // finally replace the given filename's parent_path with the equivalent canonical path
    _filename = canonical_path / _filename.filename();
  }
}

/***/
void StreamHandler::write(fmt_buffer_t const& formatted_log_message, quill::TransitEvent const& log_event)
{
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
}

/***/
void StreamHandler::flush() noexcept { fflush(_file); }

/***/
fs::path const& StreamHandler::filename() const noexcept { return _filename; }

/***/
StreamHandler::StreamHandlerType StreamHandler::stream_handler_type() const noexcept
{
  if (_file == stdout)
  {
    return StreamHandler::StreamHandlerType::Stdout;
  }
  else if (_file == stderr)
  {
    return StreamHandler::StreamHandlerType::Stderr;
  }
  else
  {
    return StreamHandler::StreamHandlerType::File;
  }
}
} // namespace quill