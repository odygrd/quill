#include "quill/handlers/StreamHandler.h"
#include <stdexcept>

namespace quill
{
/***/
StreamHandler::StreamHandler(std::string stream) : _filename(std::move(stream))
{
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
}

/***/
StreamHandler::StreamHandler(FILE* file_pointer, char const* filename)
  : _filename(std::string { filename }), _file(file_pointer)
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
std::string StreamHandler::filename() { return _filename; }
} // namespace quill