#include "quill/handlers/StreamHandler.h"
#include <stdexcept>

#if defined(_WIN32) && defined(QUILL_WCHAR_FILENAMES)
  #include <codecvt>
#endif

#include <codecvt>
namespace quill
{
/***/
StreamHandler::StreamHandler(std::string const& stream)
{
  if (stream == std::string{"stdout"})
  {
    _file = stdout;
  }
  else if (stream == std::string{"stderr"})
  {
    _file = stderr;
  }
  else
  {
    throw std::runtime_error("Invalid StreamHandler constructor value");
  }

  // Store it also as filename but first check if we have to convert to wstring because of filename_t
#if defined(_WIN32) && defined(QUILL_WCHAR_FILENAMES)
  // In this case filename_t will be wstring so convert first

  using convert_t = std::codecvt_utf8<wchar_t>;
  std::wstring_convert<convert_t, wchar_t> converter;

  _filename = converter.from_bytes(stream);
#else
  _filename = stream;
#endif
}

/***/
StreamHandler::StreamHandler(FILE* file_pointer, filename_t filename)
  : _filename(std::move(filename)), _file(file_pointer)
{
}

/***/
void StreamHandler::emit(PatternFormatter::log_record_memory_buffer const& formatted_log_record)
{
  detail::fwrite_fully(formatted_log_record.data(), sizeof(char), formatted_log_record.size(), _file);
}

/***/
void StreamHandler::flush() { fflush(_file); }

/***/
filename_t const& StreamHandler::filename() const noexcept { return _filename; }
} // namespace quill