#include "quill/handlers/RotatingFileHandler.h"
#include "quill/QuillError.h"                // for QUILL_THROW, QuillError
#include "quill/detail/misc/Common.h"        // for QUILL_UNLIKELY
#include "quill/detail/misc/FileUtilities.h" // for append_index_to_filename
#include "quill/detail/misc/Os.h"            // for rename_file
#include "quill/handlers/StreamHandler.h"    // for StreamHandler

namespace
{
/***/
std::pair<std::chrono::hours, std::chrono::minutes> default_rotation_time_daily() noexcept
{
  return std::make_pair(std::chrono::hours{std::numeric_limits<std::chrono::hours::rep>::max()},
                        std::chrono::minutes{std::numeric_limits<std::chrono::hours::rep>::max()});
}

/***/
std::pair<std::chrono::hours, std::chrono::minutes> parse_at_time_format(std::string const& at_time)
{
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(at_time);
  while (std::getline(tokenStream, token, ':'))
  {
    tokens.push_back(token);
  }

  if (tokens.size() != 2)
  {
    QUILL_THROW(quill::QuillError{"Invalid at_time value format. The format should be `HH:MM`."});
  }
  else
  {
    for (auto const& parsed_token : tokens)
    {
      if (parsed_token.size() != 2)
      {
        QUILL_THROW(
          quill::QuillError{"Invalid at_time value format. Each component of the time (HH and MM) "
                            "should be two digits."});
      }
    }
  }

  auto at_time_tp =
    std::make_pair(std::chrono::hours{std::stoi(tokens[0])}, std::chrono::minutes{std::stoi(tokens[1])});

  if ((at_time_tp.first > std::chrono::hours{23}) || (at_time_tp.second > std::chrono::minutes{59}))
  {
    QUILL_THROW(quill::QuillError(
      "Invalid rotation values. The hour value should be between 00 and 23, and the "
      "minute value should be between 00 and 59."));
  }

  return at_time_tp;
}

/***/
uint64_t calculate_initial_rotation_tp(uint64_t start_time_ns, quill::RotatingFileHandlerConfig const& config)
{
  time_t time_now = static_cast<time_t>(start_time_ns) / 1000000000;
  tm date;

  // here we do this because of `at_time` that might have specified the time in UTC
  if (config.timezone() == quill::Timezone::GmtTime)
  {
    quill::detail::gmtime_rs(&time_now, &date);
  }
  else
  {
    quill::detail::localtime_rs(&time_now, &date);
  }

  // update to the desired date
  if (config.rotation_frequency() == quill::RotatingFileHandlerConfig::RotationFrequency::Minutely)
  {
    date.tm_min += 1;
    date.tm_sec = 0;
  }
  else if (config.rotation_frequency() == quill::RotatingFileHandlerConfig::RotationFrequency::Hourly)
  {
    date.tm_hour += 1;
    date.tm_min = 0;
    date.tm_sec = 0;
  }
  else if (config.rotation_frequency() == quill::RotatingFileHandlerConfig::RotationFrequency::Daily)
  {
    date.tm_hour = static_cast<decltype(date.tm_hour)>(config.rotation_at_time_daily().first.count());
    date.tm_min = static_cast<decltype(date.tm_min)>(config.rotation_at_time_daily().second.count());
    date.tm_sec = 0;
  }
  else
  {
    QUILL_THROW(quill::QuillError{"Invalid rotation frequency"});
  }

  // convert back to timestamp
  time_t const rotation_time =
    (config.timezone() == quill::Timezone::GmtTime) ? quill::detail::timegm(&date) : std::mktime(&date);

  uint64_t rotation_time_seconds = (rotation_time > time_now)
    ? static_cast<uint64_t>(rotation_time)
    : static_cast<uint64_t>(rotation_time + std::chrono::seconds{std::chrono::hours{24}}.count());

  return static_cast<uint64_t>(std::chrono::nanoseconds{std::chrono::seconds{rotation_time_seconds}}.count());
}

/***/
uint64_t calculate_rotation_tp(uint64_t rotation_timestamp_ns, quill::RotatingFileHandlerConfig const& config)
{
  if (config.rotation_frequency() == quill::RotatingFileHandlerConfig::RotationFrequency::Minutely)
  {
    return rotation_timestamp_ns +
      static_cast<uint64_t>(
             std::chrono::nanoseconds{std::chrono::minutes{config.rotation_interval()}}.count());
  }
  else if (config.rotation_frequency() == quill::RotatingFileHandlerConfig::RotationFrequency::Hourly)
  {
    return rotation_timestamp_ns +
      static_cast<uint64_t>(std::chrono::nanoseconds{std::chrono::hours{config.rotation_interval()}}.count());
  }
  else if (config.rotation_frequency() == quill::RotatingFileHandlerConfig::RotationFrequency::Daily)
  {
    return rotation_timestamp_ns + std::chrono::nanoseconds{std::chrono::hours{24}}.count();
  }

  QUILL_THROW(quill::QuillError{"Invalid rotation frequency"});
}

/***/
quill::fs::path get_filename(quill::fs::path filename, uint32_t index, std::string const& date_time)
{
  if (!date_time.empty())
  {
    filename = quill::detail::append_string_to_filename(filename, date_time);
  }

  if (index > 0)
  {
    filename = quill::detail::append_index_to_filename(filename, index);
  }

  return filename;
}
} // namespace

namespace quill
{
/***/
RotatingFileHandlerConfig::RotatingFileHandlerConfig()
  : _rotation_at_time_daily{default_rotation_time_daily()}
{
}

/***/
void RotatingFileHandlerConfig::set_rotation_max_file_size(size_t value)
{
  if (value < 512)
  {
    QUILL_THROW(QuillError{"rotation_max_file_size must be greater than or equal to 512 bytes"});
  }

  _rotation_max_file_size = value;
}

/***/
void RotatingFileHandlerConfig::set_rotation_frequency_and_interval(char frequency, uint32_t interval)
{
  if (frequency == 'M' || frequency == 'm')
  {
    _rotation_frequency = RotationFrequency::Minutely;
  }
  else if (frequency == 'H' || frequency == 'h')
  {
    _rotation_frequency = RotationFrequency::Hourly;
  }
  else
  {
    QUILL_THROW(QuillError{
      "Invalid frequency. Valid values are 'M' or 'm' for minutes or 'H' or 'h' for hours"});
  }

  if (interval == 0)
  {
    QUILL_THROW(QuillError{"interval must be set to a value greater than 0"});
  }

  _rotation_interval = interval;
  _rotation_at_time_daily = default_rotation_time_daily();
}

/***/
void RotatingFileHandlerConfig::set_rotation_time_daily(std::string const& at_time)
{
  _rotation_frequency = RotationFrequency::Daily;
  _rotation_interval = 0;
  _rotation_at_time_daily = parse_at_time_format(at_time);
}

/***/
void RotatingFileHandlerConfig::set_max_backup_files(uint32_t value) { _max_backup_files = value; }

/***/
void RotatingFileHandlerConfig::set_overwrite_rolled_files(bool value)
{
  _overwrite_rolled_files = value;
}

/***/
void RotatingFileHandlerConfig::set_remove_old_files(bool value) { _remove_old_files = value; }

/***/
void RotatingFileHandlerConfig::set_rotation_naming_scheme(RotationNamingScheme value)
{
  _rotation_naming_scheme = value;
}

/***/
RotatingFileHandler::RotatingFileHandler(
  fs::path const& filename, RotatingFileHandlerConfig const& config, FileEventNotifier file_event_notifier,
  std::chrono::system_clock::time_point start_time /* = std::chrono::system_clock::now() */)
  : FileHandler(filename, static_cast<FileHandlerConfig const&>(config), std::move(file_event_notifier), false),
    _config(config)
{
  uint64_t const today_timestamp_ns = static_cast<uint64_t>(
    std::chrono::duration_cast<std::chrono::nanoseconds>(start_time.time_since_epoch()).count());

  _clean_and_recover_files(filename, _config.open_mode(), today_timestamp_ns);

  if (_config.rotation_frequency() != RotatingFileHandlerConfig::RotationFrequency::Disabled)
  {
    // Calculate next rotation time
    _next_rotation_time = calculate_initial_rotation_tp(
      static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(start_time.time_since_epoch()).count()),
      config);
  }

  // Open file for logging
  open_file(_filename, _config.open_mode());
  _open_file_timestamp = static_cast<uint64_t>(
    std::chrono::duration_cast<std::chrono::nanoseconds>(start_time.time_since_epoch()).count());

  _created_files.emplace_front(_filename, 0, std::string{});

  if (!is_null())
  {
    _file_size = detail::file_size(_filename);
  }
}

/***/
void RotatingFileHandler::write(fmt_buffer_t const& formatted_log_message, quill::TransitEvent const& log_event)
{
  if (is_null())
  {
    StreamHandler::write(formatted_log_message, log_event);
    return;
  }

  bool time_rotation = false;

  if (_config.rotation_frequency() != RotatingFileHandlerConfig::RotationFrequency::Disabled)
  {
    // Check if we need to rotate based on time
    time_rotation = _time_rotation(log_event.header.timestamp);
  }

  if (!time_rotation && _config.rotation_max_file_size() != 0)
  {
    // Check if we need to rotate based on size
    _size_rotation(formatted_log_message.size(), log_event.header.timestamp);
  }

  // write to file
  StreamHandler::write(formatted_log_message, log_event);
  _file_size += formatted_log_message.size();
}

/***/
bool RotatingFileHandler::_time_rotation(uint64_t record_timestamp_ns)
{
  if (record_timestamp_ns >= _next_rotation_time)
  {
    _rotate_files(record_timestamp_ns);
    _next_rotation_time = calculate_rotation_tp(record_timestamp_ns, _config);
    return true;
  }

  return false;
}

void RotatingFileHandler::_size_rotation(size_t log_msg_size, uint64_t record_timestamp_ns)
{
  // Calculate the new size of the file
  if (_file_size + log_msg_size > _config.rotation_max_file_size())
  {
    _rotate_files(record_timestamp_ns);
  }
}

void RotatingFileHandler::_rotate_files(uint64_t record_timestamp_ns)
{
  if ((_created_files.size() > _config.max_backup_files()) && !_config.overwrite_rolled_files())
  {
    // We have reached the max number of backup files, and we are not allowed to overwrite the
    // oldest file. We will stop rotating
    return;
  }

  FileHandler::flush();

  if (detail::file_size(_filename) <= 0)
  {
    // Also check the file size is > 0  to better deal with full disk
    return;
  }

  close_file();

  // datetime_suffix will be empty if we are using the default naming scheme
  std::string datetime_suffix;
  if (_config.rotation_naming_scheme() == RotatingFileHandlerConfig::RotationNamingScheme::Date)
  {
    datetime_suffix = quill::detail::get_datetime_string(_open_file_timestamp, _config.timezone(), false);
  }
  else if (_config.rotation_naming_scheme() == RotatingFileHandlerConfig::RotationNamingScheme::DateAndTime)
  {
    datetime_suffix = quill::detail::get_datetime_string(_open_file_timestamp, _config.timezone(), true);
  }

  // We need to rotate the files and rename them with an index
  for (auto it = _created_files.rbegin(); it != _created_files.rend(); ++it)
  {
    // Create each existing filename on disk with the existing index.
    // when the index is 0 we want to rename the latest file
    fs::path existing_file;
    fs::path renamed_file;

    existing_file = get_filename(it->base_filename, it->index, it->date_time);

    // increment the index if needed and rename the file
    uint32_t index_to_use = it->index;

    if (_config.rotation_naming_scheme() == RotatingFileHandlerConfig::RotationNamingScheme::Index ||
        it->date_time == datetime_suffix)
    {
      // we are rotating and incrementing the index, or we have another file with the same date_time suffix
      index_to_use += 1;

      renamed_file = get_filename(it->base_filename, index_to_use, datetime_suffix);

      it->index = index_to_use;
      it->date_time = datetime_suffix;

      quill::detail::rename_file(existing_file, renamed_file);
    }
    else if (it->date_time.empty())
    {
      // we are renaming the latest file
      index_to_use = it->index;

      renamed_file = get_filename(it->base_filename, index_to_use, datetime_suffix);

      it->index = index_to_use;
      it->date_time = datetime_suffix;

      quill::detail::rename_file(existing_file, renamed_file);
    }
  }

  // Check if we have too many files in the queue remove_file the oldest one
  if (_created_files.size() > _config.max_backup_files())
  {
    // remove_file that file from the system and also pop it from the queue
    fs::path const removed_file = get_filename(
      _created_files.back().base_filename, _created_files.back().index, _created_files.back().date_time);
    detail::remove_file(removed_file);
    _created_files.pop_back();
  }

  // add the current file back to the list with index 0
  _created_files.emplace_front(_filename, 0, std::string{});

  // Open file for logging
  open_file(_filename, "w");
  _open_file_timestamp = record_timestamp_ns;
  _file_size = 0;
}

/***/
void RotatingFileHandler::_clean_and_recover_files(fs::path const& filename, std::string const& open_mode,
                                                   uint64_t today_timestamp_ns)
{
  if ((_config.rotation_naming_scheme() != RotatingFileHandlerConfig::RotationNamingScheme::Index) &&
      (_config.rotation_naming_scheme() != RotatingFileHandlerConfig::RotationNamingScheme::Date))
  {
    // clean and recover is only supported for index and date naming scheme, when using DateAndTime
    // there are no collisions in the filenames
    return;
  }

  // if we are starting in "w" mode, then we also should clean all previous log files of the previous run
  if (_config.remove_old_files() && (open_mode == "w"))
  {
    for (const auto& entry : fs::directory_iterator(fs::current_path() / filename.parent_path()))
    {
      if (entry.path().extension().string() != filename.extension().string())
      {
        // we only check for the files of the same extension to remove
        continue;
      }

      // is_directory() does not exist in std::experimental::filesystem
      if (entry.path().filename().string().find(filename.stem().string() + ".") != 0)
      {
        // expect to find filename.stem().string() exactly at the start of the filename
        continue;
      }

      if (_config.rotation_naming_scheme() == RotatingFileHandlerConfig::RotationNamingScheme::Index)
      {
        fs::remove(entry);
      }
      else if (_config.rotation_naming_scheme() == RotatingFileHandlerConfig::RotationNamingScheme::Date)
      {
        // Find the first dot in the filename
        // stem will be something like `logfile.1`
        size_t pos = entry.path().stem().string().find_last_of('.');
        if (pos != std::string::npos)
        {
          // Get the today's date, we won't remove the files of the previous dates as they won't collide
          std::string const today_date =
            quill::detail::get_datetime_string(today_timestamp_ns, _config.timezone(), false);

          std::string const index_or_date =
            entry.path().stem().string().substr(pos + 1, entry.path().stem().string().length());

          if ((index_or_date.length() >= 8) && (index_or_date == today_date))
          {
            // assume it is a date, no need to find the index
            if (index_or_date == today_date)
            {
              fs::remove(entry);
            }
          }
          else
          {
            // assume it is an index
            // Find the second last dot to get the date
            std::string const filename_with_date = entry.path().filename().string().substr(0, pos);
            size_t second_last = filename_with_date.find_last_of('.');

            if (second_last != std::string::npos)
            {
              std::string const date_part =
                filename_with_date.substr(second_last + 1, filename_with_date.length());

              if (date_part == today_date)
              {
                fs::remove(entry);
              }
            }
          }
        }
      }
    }
  }
  else if (open_mode == "a")
  {
    // we need to recover the index from the existing files
    for (const auto& entry : fs::directory_iterator(fs::current_path() / filename.parent_path()))
    {
      // is_directory() does not exist in std::experimental::filesystem
      if (entry.path().extension().string() != filename.extension().string())
      {
        // we only check for the files of the same extension to remove
        continue;
      }

      // is_directory() does not exist in std::experimental::filesystem
      if (entry.path().filename().string().find(filename.stem().string() + ".") != 0)
      {
        // expect to find filename.stem().string() exactly at the start of the filename
        continue;
      }

      std::string const extension = entry.path().extension().string(); // e.g. ".log"

      // stem will be something like `logfile.1`
      size_t pos = entry.path().stem().string().find_last_of('.');
      if (pos != std::string::npos)
      {
        if (_config.rotation_naming_scheme() == RotatingFileHandlerConfig::RotationNamingScheme::Index)
        {
          std::string const index =
            entry.path().stem().string().substr(pos + 1, entry.path().stem().string().length());

          std::string const current_filename = entry.path().filename().string().substr(0, pos) + extension;
          fs::path current_file = entry.path().parent_path();
          current_file.append(current_filename);

          // Attempt to convert the index to a number
          QUILL_TRY
          {
            _created_files.emplace_front(current_file, static_cast<uint32_t>(std::stoul(index)), std::string{});
          }
          QUILL_CATCH_ALL() { continue; }
        }
        else if (_config.rotation_naming_scheme() == RotatingFileHandlerConfig::RotationNamingScheme::Date)
        {
          // Get the today's date, we won't remove the files of the previous dates as they won't collide
          std::string const today_date =
            quill::detail::get_datetime_string(today_timestamp_ns, _config.timezone(), false);

          std::string const index_or_date =
            entry.path().stem().string().substr(pos + 1, entry.path().stem().string().length());

          if ((index_or_date.length() >= 8) && (index_or_date == today_date))
          {
            // assume it is a date, no need to find the index
            std::string const current_filename = entry.path().filename().string().substr(0, pos) + extension;
            fs::path current_file = entry.path().parent_path();
            current_file.append(current_filename);

            _created_files.emplace_front(current_file, 0, index_or_date);
          }
          else
          {
            // assume it is an index
            // Find the second last dot to get the date
            std::string const filename_with_date = entry.path().filename().string().substr(0, pos);
            size_t second_last = filename_with_date.find_last_of('.');

            if (second_last != std::string::npos)
            {
              std::string const date_part =
                filename_with_date.substr(second_last + 1, filename_with_date.length());

              if (date_part == today_date)
              {
                std::string const current_filename = filename_with_date.substr(0, second_last) + extension;
                fs::path current_file = entry.path().parent_path();
                current_file.append(current_filename);

                // Attempt to convert the index to a number
                QUILL_TRY
                {
                  _created_files.emplace_front(
                    current_file, static_cast<uint32_t>(std::stoul(index_or_date)), date_part);
                }
                QUILL_CATCH_ALL() { continue; }
              }
            }
          }
        }
      }
    }

    // finally we need to sort the deque
    std::sort(_created_files.begin(), _created_files.end(),
              [](FileInfo const& a, FileInfo const& b) { return a.index < b.index; });
  }
}
} // namespace quill
