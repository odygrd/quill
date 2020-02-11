#include "quill/handlers/StreamHandler.h"
#include "quill/detail/misc/Utilities.h"
#include <stdexcept>

namespace quill
{
/***/
StreamHandler::StreamHandler(filename_t stream) : _filename(std::move(stream))
{
#if defined(_WIN32)
  if (_filename == std::wstring{L"stdout"})
  {
    _file = stdout;
  }
  else if (_filename == std::wstring{L"stderr"})
  {
    _file = stderr;
  }
  else
  {
    throw std::runtime_error("Invalid StreamHandler constructor value");
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
  else
  {
    throw std::runtime_error("Invalid StreamHandler constructor value");
  }
#endif
}

/***/
StreamHandler::StreamHandler(FILE* file_pointer, filename_t filename)
  : _filename(std::move(filename)), _file(file_pointer)
{
}

/***/
void StreamHandler::emit(fmt::memory_buffer const& formatted_log_record)
{
  detail::fwrite_fully(formatted_log_record.data(), sizeof(char), formatted_log_record.size(), _file);
}

/***/
void StreamHandler::flush() { fflush(_file); }

/***/
filename_t const& StreamHandler::filename() const noexcept { return _filename; }
} // namespace quill