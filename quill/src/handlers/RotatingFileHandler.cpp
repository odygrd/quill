#include "quill/handlers/RotatingFileHandler.h"
#include "quill/QuillError.h"                // for QUILL_THROW, QuillError
#include "quill/detail/misc/FileUtilities.h" // for append_index_to_filename
#include "quill/detail/misc/Macros.h"        // for QUILL_UNLIKELY
#include "quill/handlers/StreamHandler.h"    // for StreamHandler
#include <cerrno>                            // for errno
#include <cstdio>                            // for fclose
#include <ostream>                           // for operator<<, basic_ostre...

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
    std::ostringstream error_msg;
    error_msg << "failed to close previous log file during rotation, with error message errno: \"" << errno << "\"";
    QUILL_THROW(QuillError{error_msg.str()});
  }

  _current_filename = detail::file_utilities::append_index_to_filename(_filename, _index);
  ++_index;
  _file = detail::file_utilities::open(_current_filename, "a");
}
} // namespace quill