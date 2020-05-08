#include "quill/handlers/DailyFileHandler.h"
#include "quill/QuillError.h"                // for QuillError, QUILL_THROW
#include "quill/detail/misc/Common.h"        // for filename_t
#include "quill/detail/misc/FileUtilities.h" // for append_date_to_filename
#include "quill/detail/misc/Macros.h"        // for QUILL_UNLIKELY
#include "quill/detail/misc/Os.h"            // for localtime_rs
#include "quill/handlers/StreamHandler.h"    // for StreamHandler
#include <cerrno>                            // for errno
#include <cstdio>                            // for fclose
#include <ctime>                             // for mktime, time_t
#include <ostream>                           // for operator<<, basic_ostre...

namespace quill
{

/***/
DailyFileHandler::DailyFileHandler(filename_t const& base_filename, std::chrono::hours rotation_hour,
                                   std::chrono::minutes rotation_minute)
  : FileHandler(base_filename), _rotation_hour(rotation_hour), _rotation_minute(rotation_minute)
{
  if ((_rotation_hour > std::chrono::hours{23}) || (_rotation_minute > std::chrono::minutes{59}))
  {
    QUILL_THROW(QuillError("Invalid rotation values"));
  }

  // Generate the filename and open
  _current_filename = detail::file_utilities::append_date_to_filename(_filename);
  _file = detail::file_utilities::open(_current_filename, "a");
  _update_rotation_tp();
}

/***/
void DailyFileHandler::write(fmt::memory_buffer const& formatted_log_record, std::chrono::nanoseconds log_record_timestamp)
{
  bool should_rotate = log_record_timestamp >= _rotation_tp.time_since_epoch();
  if (QUILL_UNLIKELY(should_rotate))
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

    // Generate a new filename - rotating
    _current_filename = detail::file_utilities::append_date_to_filename(_filename);
    _file = detail::file_utilities::open(_current_filename, "a");
    _update_rotation_tp();
  }

  // write to file
  StreamHandler::write(formatted_log_record, log_record_timestamp);
}

/***/
void DailyFileHandler::_update_rotation_tp() noexcept
{
  // Get the time now as tm
  std::chrono::system_clock::time_point const now = std::chrono::system_clock::now();
  time_t tnow = std::chrono::system_clock::to_time_t(now);
  tm date;
  detail::localtime_rs(&tnow, &date);

  // update to the desired date
  date.tm_hour = static_cast<int>(_rotation_hour.count());
  date.tm_min = static_cast<int>(_rotation_minute.count());
  date.tm_sec = 0;

  // convert back to timestamp
  std::chrono::system_clock::time_point const rotation_time =
    std::chrono::system_clock::from_time_t(std::mktime(&date));

  _rotation_tp = (rotation_time > now) ? rotation_time : rotation_time + std::chrono::hours(24);
}
} // namespace quill