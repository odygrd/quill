#include "quill/handlers/TimeRotatingFileHandler.h"
#include "quill/QuillError.h"                // for QuillError, QUILL_THROW
#include "quill/detail/misc/Common.h"        // for filename_t
#include "quill/detail/misc/FileUtilities.h" // for append_date_to_filename
#include "quill/detail/misc/Macros.h"        // for QUILL_UNLIKELY
#include "quill/detail/misc/Os.h"            // for localtime_rs
#include "quill/handlers/StreamHandler.h"    // for StreamHandler
#include <cerrno>                            // for errno
#include <chrono>
#include <cstdio>  // for fclose
#include <ctime>   // for mktime, time_t
#include <ostream> // for operator<<, basic_ostre...

namespace quill
{

/***/
TimeRotatingFileHandler::TimeRotatingFileHandler(filename_t const& base_filename, std::string const& mode,
                                                 std::string when, uint32_t interval, uint32_t backup_count,
                                                 bool utc, std::string const& at_time)
  : FileHandler(base_filename), _when(std::move(when)), _interval(interval), _backup_count(backup_count), _utc(utc)
{
  if ((_when != std::string{"M"}) && _when != std::string{"H"} && _when != std::string{"daily"})
  {
    QUILL_THROW(
      QuillError{"Invalid when value for TimeRotatingFileHandler. Valid values are 'S', 'M', 'H' "
                 "or 'daily'"});
  }

  std::chrono::hours at_time_hours{0};
  std::chrono::minutes at_time_minutes{0};

  // Note at_time is only used when 'daily' is passed otherwise ignored

  // Split the string
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(at_time);
  while (std::getline(tokenStream, token, ':'))
  {
    tokens.push_back(token);
  }

  if (tokens.size() != 2)
  {
    QUILL_THROW(QuillError{"Invalid at_time value."});
  }

  at_time_hours = std::chrono::hours(std::stoi(tokens[0]));
  at_time_minutes = std::chrono::minutes(std::stoi(tokens[1]));

  if ((at_time_hours > std::chrono::hours{23}) || (at_time_minutes > std::chrono::minutes{59}))
  {
    QUILL_THROW(QuillError("Invalid rotation values"));
  }

  // Calculate next rotation time
  _file_creation_time = std::chrono::system_clock::now();
  _next_rotation_time =
    _calculate_initial_rotation_tp(_file_creation_time, _when, utc, at_time_hours, at_time_minutes);

  // Open file for logging
  _file = detail::file_utilities::open(_filename, mode);
}

/***/
void TimeRotatingFileHandler::write(fmt::memory_buffer const& formatted_log_record,
                                    std::chrono::nanoseconds log_record_timestamp)
{
  bool const should_rotate = (log_record_timestamp >= _next_rotation_time.time_since_epoch());

  if (QUILL_UNLIKELY(should_rotate))
  {
    if (_file)
    {
      // close the previous file
      int const res = fclose(_file);

      if (QUILL_UNLIKELY(res != 0))
      {
        std::ostringstream error_msg;
        error_msg
          << "failed to close previous log file during rotation, with error message errno: \""
          << errno << "\"";
        QUILL_THROW(QuillError{error_msg.str()});
      }
    }

    filename_t const previous_file = _filename;
    bool const append_time_to_filename = true;
    filename_t const new_file = detail::file_utilities::append_date_to_filename(
      _filename, _file_creation_time, append_time_to_filename, _utc);

    quill::detail::rename(previous_file, new_file);

    // Also store the file name in a queue to remove it later if we exceed backup count
    _created_files.push(new_file);

    // If we have too many files in the queue remove the oldest one
    if (_created_files.size() > _backup_count)
    {
      // remove that file from the system and also pop it from the queue
      detail::remove(_created_files.front());
      _created_files.pop();
    }

    // Calculate next rotation time and start writing the new log
    _file_creation_time = std::chrono::system_clock::now();
    _next_rotation_time = _calculate_rotation_tp(_file_creation_time, _when, _interval);

    // Open file for logging
    _file = detail::file_utilities::open(_filename, "w");
  }

  // write to file
  StreamHandler::write(formatted_log_record, log_record_timestamp);
}

/***/
std::chrono::system_clock::time_point TimeRotatingFileHandler::_calculate_initial_rotation_tp(
  std::chrono::system_clock::time_point time_now, std::string const& when, bool utc,
  std::chrono::hours at_time_hours, std::chrono::minutes at_time_minutes) noexcept
{
  time_t tnow = std::chrono::system_clock::to_time_t(time_now);
  tm date;

  // utc is used because of at_time that might specify the time in UTC
  if (utc)
  {
    detail::gmtime_rs(&tnow, &date);
  }
  else
  {
    detail::localtime_rs(&tnow, &date);
  }

  // update to the desired date
  if (when == std::string{"M"})
  {
    date.tm_min += 1;
    date.tm_sec = 0;
  }
  else if (when == std::string{"H"})
  {
    date.tm_hour += 1;
    date.tm_min = 0;
    date.tm_sec = 0;
  }
  else if (when == std::string{"daily"})
  {
    date.tm_hour = static_cast<int>(at_time_hours.count());
    date.tm_min = static_cast<int>(at_time_minutes.count());
    date.tm_sec = 0;
  }

  // convert back to timestamp
  std::chrono::system_clock::time_point const rotation_time = utc
    ? std::chrono::system_clock::from_time_t(quill::detail::timegm(&date))
    : std::chrono::system_clock::from_time_t(std::mktime(&date));

  return (rotation_time > time_now) ? rotation_time : rotation_time + std::chrono::hours(24);
}

/***/
std::chrono::system_clock::time_point TimeRotatingFileHandler::_calculate_rotation_tp(
  std::chrono::system_clock::time_point time_now, std::string const& when, uint32_t interval) noexcept
{
  if (when == std::string{"M"})
  {
    return time_now + std::chrono::minutes{interval};
  }
  else if (when == std::string{"H"})
  {
    return time_now + std::chrono::hours{interval};
  }
  else if (when == std::string{"daily"})
  {
    return time_now + std::chrono::hours{24};
  }

  return std::chrono::system_clock::time_point{};
}
} // namespace quill