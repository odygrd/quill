#include "quill/handlers/RotatingFileHandler.h"
#include "quill/QuillError.h"                // for QUILL_THROW, QuillError
#include "quill/detail/misc/FileUtilities.h" // for append_index_to_filename
#include "quill/detail/misc/Macros.h"        // for QUILL_UNLIKELY
#include "quill/detail/misc/Os.h"            // for rename
#include "quill/handlers/StreamHandler.h"    // for StreamHandler
#include <cerrno>                            // for errno
#include <cstdio>                            // for fclose
#include <ostream>                           // for operator<<, basic_ostre...

namespace quill
{
/***/
RotatingFileHandler::RotatingFileHandler(filename_t const& base_filename, std::string const& mode,
                                         size_t max_bytes, uint32_t backup_count)
  : FileHandler(base_filename), _max_bytes(max_bytes), _backup_count(backup_count)
{
  _file = detail::file_utilities::open(_filename, mode);
  _current_size = detail::file_utilities::file_size(_file);
}

/***/
void RotatingFileHandler::write(fmt::memory_buffer const& formatted_log_record,
                                std::chrono::nanoseconds log_record_timestamp, LogLevel log_message_severity)
{
  _current_size += formatted_log_record.size();

  if (_current_size > _max_bytes)
  {
    _rotate();
    _current_size = formatted_log_record.size();
  }

  // write to file
  StreamHandler::write(formatted_log_record, log_record_timestamp, log_message_severity);
}

/***/
void RotatingFileHandler::_rotate()
{
  if (_current_index >= _backup_count)
  {
    // we can not rotate anymore, do nothing
    return;
  }

  if (_file)
  {
    // close the previous file
    int const res = fclose(_file);
    if (QUILL_UNLIKELY(res != 0))
    {
      std::ostringstream error_msg;
      error_msg << "failed to close previous log file during rotation, with error message errno: \""
                << errno << "\"";
      QUILL_THROW(QuillError{error_msg.str()});
    }
  }

  // if we have more than 2 files we need to start renaming recursively
  for (uint32_t i = _current_index; i >= 1; --i)
  {
    filename_t const previous_file = detail::file_utilities::append_index_to_filename(_filename, i);
    filename_t const new_file = detail::file_utilities::append_index_to_filename(_filename, i + 1);

    quill::detail::rename(previous_file, new_file);
  }

  // then we will always rename the base filename to 1
  filename_t const previous_file = _filename;
  filename_t const new_file = detail::file_utilities::append_index_to_filename(_filename, 1);

  quill::detail::rename(previous_file, new_file);

  // Increment the rotation index
  ++_current_index;

  // Now reopen the base filename for writing again
  _file = detail::file_utilities::open(_filename, "w");
}
} // namespace quill