#include "quill/handlers/StreamHandler.h"
#include "quill/detail/misc/FileUtilities.h" // for fwrite_fully
#include <string>                            // for allocator, operator==
#include <utility>                           // for move

namespace quill
{
/***/
StreamHandler::StreamHandler(filename_t stream, FILE* file /* = nullptr */)
  : _filename(std::move(stream)), _file(file)
{
  // reserve stdout and stderr as filenames
#if defined(_WIN32)
  if (_filename == std::wstring{L"stdout"})
  {
    _file = stdout;
  }
  else if (_filename == std::wstring{L"stderr"})
  {
    _file = stderr;
  }
#else
  if (_filename == std::string{"stdout"})
  {
    _file = stdout;
  }
  else if (_filename == std::string{"stderr"})
  {
    _file = stderr;
  }
#endif
}

/***/
void StreamHandler::write(fmt::memory_buffer const& formatted_log_record, std::chrono::nanoseconds, LogLevel)
{
  detail::file_utilities::fwrite_fully(formatted_log_record.data(), sizeof(char),
                                       formatted_log_record.size(), _file);
}

/***/
void StreamHandler::flush() noexcept { fflush(_file); }

/***/
filename_t const& StreamHandler::filename() const noexcept { return _filename; }

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