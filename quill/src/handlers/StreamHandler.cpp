#include "quill/handlers/StreamHandler.h"

#include "quill/detail/misc/FileUtilities.h"

namespace quill
{
/***/
StreamHandler::StreamHandler(filename_t stream) : _filename(std::move(stream))
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
void StreamHandler::write(fmt::memory_buffer const& formatted_log_record, std::chrono::nanoseconds)
{
  detail::file_utilities::fwrite_fully(formatted_log_record.data(), sizeof(char),
                                       formatted_log_record.size(), _file);
}

/***/
void StreamHandler::flush() noexcept { fflush(_file); }

/***/
filename_t const& StreamHandler::filename() const noexcept { return _filename; }
} // namespace quill