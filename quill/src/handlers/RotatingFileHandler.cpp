#include "quill/handlers/RotatingFileHandler.h"
#include "quill/detail/misc/FileUtilities.h"
#include "quill/detail/misc/Macros.h"

namespace quill
{
/***/
RotatingFileHandler::RotatingFileHandler(filename_t const& base_filename, std::size_t max_bytes)
  : FileHandler(base_filename), _max_bytes(max_bytes)
{
  // Generate the filename and open
  _current_filename = detail::file_utilities::append_index_to_filename(base_filename, _index);
  ++_index;
  _file = detail::file_utilities::open(_current_filename, "a");
  _current_size = detail::file_utilities::file_size(_file);
}

/***/
void RotatingFileHandler::write(fmt::memory_buffer const& formatted_log_record, std::chrono::nanoseconds log_record_timestamp)
{
  _current_size += formatted_log_record.size();

  if (_current_size > _max_bytes)
  {
    _rotate();
    _current_size = formatted_log_record.size();
  }

  // write to file
  StreamHandler::write(formatted_log_record, log_record_timestamp);
}

/***/
void RotatingFileHandler::_rotate()
{
  // close the previous file
  int res = 1;

  if (_file)
  {
    res = fclose(_file);
  }

  if (QUILL_UNLIKELY(res != 0))
  {
    throw std::system_error(errno, std::system_category());
  }

  _current_filename = detail::file_utilities::append_index_to_filename(_filename, _index);
  ++_index;
  _file = detail::file_utilities::open(_current_filename, "a");
}
} // namespace quill