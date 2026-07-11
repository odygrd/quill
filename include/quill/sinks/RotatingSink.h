/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Common.h"
#include "quill/core/Filesystem.h"
#include "quill/core/LogLevel.h"
#include "quill/core/QuillError.h"
#include "quill/core/TimeUtilities.h"
#include "quill/sinks/FileSink.h"
#include "quill/sinks/StreamSink.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <deque>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

QUILL_BEGIN_NAMESPACE

/** Forward Declaration **/
QUILL_BEGIN_EXPORT

class MacroMetadata;

/**
 * @brief The configuration options for the RotatingSink
 */
class RotatingFileSinkConfig : public FileSinkConfig
{
public:
  /**
   * @brief The frequency of log file rotation
   */
  enum class RotationFrequency : uint8_t
  {
    Disabled,
    Daily,
    Hourly,
    Minutely
  };

  /**
   * @brief The naming scheme for rotated log files
   */
  enum class RotationNamingScheme : uint8_t
  {
    Index,
    Date,
    DateAndTime
  };

  /**
   * @brief Constructs a new RotatingFileSinkConfig object.
   */
  RotatingFileSinkConfig() : _daily_rotation_time{_disabled_daily_rotation_time()} {}

  /**
   * @brief Sets the maximum file size for rotation.
   * @param value The maximum file size in bytes per file
   */
  QUILL_ATTRIBUTE_COLD void set_rotation_max_file_size(size_t value)
  {
    if (value < 512)
    {
      QUILL_THROW(QuillError{"rotation_max_file_size must be greater than or equal to 512 bytes"});
    }

    _rotation_max_file_size = value;
  }

  /**
   * @brief Sets the frequency and interval of file rotation.
   * @param frequency The frequency of file rotation ('M' for minutes, 'H' for hours)
   * @param interval The rotation interval
   */
  QUILL_ATTRIBUTE_COLD void set_rotation_frequency_and_interval(char frequency, uint32_t interval)
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
    _daily_rotation_time = _disabled_daily_rotation_time();
  }

  /**
   * @brief Sets the time of day for daily log file rotation.
   * @param daily_rotation_time_str The time of day for rotation (format: "HH:MM")
   */
  QUILL_ATTRIBUTE_COLD void set_rotation_time_daily(std::string const& daily_rotation_time_str)
  {
    _rotation_frequency = RotationFrequency::Daily;
    _rotation_interval = 0;
    _daily_rotation_time = _parse_daily_rotation_time(daily_rotation_time_str);
  }

  /**
   * @brief Sets the maximum number of log files to keep.
   * When the sink starts in append mode, rotated files left behind by previous runs are also
   * counted towards this limit and the oldest ones are removed on startup, unless
   * set_overwrite_rolled_files() is set to false.
   * @param value The maximum number of log files
   */
  QUILL_ATTRIBUTE_COLD void set_max_backup_files(uint32_t value) { _max_backup_files = value; }

  /**
   * @brief Sets whether the oldest rolled logs should be overwritten when the maximum backup count
   * is reached. If set to false, the oldest logs will not be overwritten when the maximum backup
   * count is reached, and log file rotation will stop. The default value is true.
   * @param value True to overwrite the oldest logs, false otherwise.
   */
  QUILL_ATTRIBUTE_COLD void set_overwrite_rolled_files(bool value)
  {
    _overwrite_rolled_files = value;
  }

  /**
   * @brief Sets whether previous rotated log files should be removed on process start up.
   * @note This option works only when using the mode="w"
   * This is useful to avoid conflicting file names when the process restarts and
   * FilenameAppendOption::StartDateTime was not set. The default value is true.
   * @param value True to remove old log files, false otherwise.
   */
  QUILL_ATTRIBUTE_COLD void set_remove_old_files(bool value) { _remove_old_files = value; }

  /**
   * @brief Sets the naming scheme for the rotated files.
   * The default value is 'Index'.
   * @param value The naming scheme to set.
   */
  QUILL_ATTRIBUTE_COLD void set_rotation_naming_scheme(RotationNamingScheme value)
  {
    _rotation_naming_scheme = value;
  }

  /**
   * @brief Sets whether to force rotation on file sink creation/startup.
   * When enabled, if a log file with the same name exists on startup, it will be rotated
   * according to the RotationNamingScheme before starting to write new logs.
   * This allows creating a new log file for every program run. The default value is false.
   * @param value True to force rotation on creation, false otherwise.
   */
  QUILL_ATTRIBUTE_COLD void set_rotation_on_creation(bool value) { _rotation_on_creation = value; }

  /** Getter methods **/
  QUILL_NODISCARD size_t rotation_max_file_size() const noexcept { return _rotation_max_file_size; }
  QUILL_NODISCARD uint32_t max_backup_files() const noexcept { return _max_backup_files; }
  QUILL_NODISCARD bool overwrite_rolled_files() const noexcept { return _overwrite_rolled_files; }
  QUILL_NODISCARD bool remove_old_files() const noexcept { return _remove_old_files; }
  QUILL_NODISCARD RotationFrequency rotation_frequency() const noexcept
  {
    return _rotation_frequency;
  }
  QUILL_NODISCARD uint32_t rotation_interval() const noexcept { return _rotation_interval; }
  QUILL_NODISCARD std::pair<std::chrono::hours, std::chrono::minutes> daily_rotation_time() const noexcept
  {
    return _daily_rotation_time;
  }
  QUILL_NODISCARD RotationNamingScheme rotation_naming_scheme() const noexcept
  {
    return _rotation_naming_scheme;
  }
  QUILL_NODISCARD bool rotation_on_creation() const noexcept { return _rotation_on_creation; }

private:
  /***/
  static std::pair<std::chrono::hours, std::chrono::minutes> _disabled_daily_rotation_time() noexcept
  {
    return std::make_pair(std::chrono::hours{(std::numeric_limits<std::chrono::hours::rep>::max)()},
                          std::chrono::minutes{(std::numeric_limits<std::chrono::minutes::rep>::max)()});
  }

  /***/
  static std::pair<std::chrono::hours, std::chrono::minutes> _parse_daily_rotation_time(std::string const& daily_rotation_time_str)
  {
    std::vector<std::string> tokens;
    std::string token;
    size_t start = 0, end = 0;

    while ((end = daily_rotation_time_str.find(':', start)) != std::string::npos)
    {
      token = daily_rotation_time_str.substr(start, end - start);
      tokens.push_back(token);
      start = end + 1;
    }

    // Add the last token (or the only token if there's no delimiter)
    token = daily_rotation_time_str.substr(start);
    tokens.push_back(token);

    if (tokens.size() != 2)
    {
      QUILL_THROW(
        QuillError{"Invalid daily_rotation_time_str value format. The format should be `HH:MM`."});
    }

    for (auto const& parsed_token : tokens)
    {
      if (parsed_token.size() != 2)
      {
        QUILL_THROW(QuillError{
          "Invalid daily_rotation_time_str value format. Each component of the time (HH and MM) "
          "should be two digits."});
      }
    }

    uint32_t const hours = _parse_two_digit_time_component(tokens[0]);
    uint32_t const minutes = _parse_two_digit_time_component(tokens[1]);

    auto const daily_rotation_time_str_tp =
      std::make_pair(std::chrono::hours{hours}, std::chrono::minutes{minutes});

    if ((daily_rotation_time_str_tp.first > std::chrono::hours{23}) ||
        (daily_rotation_time_str_tp.second > std::chrono::minutes{59}))
    {
      QUILL_THROW(
        QuillError("Invalid rotation values. The hour value should be between 00 and 23, and the "
                   "minute value should be between 00 and 59."));
    }

    return daily_rotation_time_str_tp;
  }

  /***/
  static uint32_t _parse_two_digit_time_component(std::string const& token)
  {
    if ((token.size() != 2) || (token[0] < '0') || (token[0] > '9') || (token[1] < '0') || (token[1] > '9'))
    {
      QUILL_THROW(QuillError{
        "Invalid daily_rotation_time_str value format. Each component of the time (HH and MM) "
        "should be two digits."});
    }

    return (static_cast<uint32_t>(token[0] - '0') * 10u) + static_cast<uint32_t>(token[1] - '0');
  }

private:
  std::pair<std::chrono::hours, std::chrono::minutes> _daily_rotation_time;
  size_t _rotation_max_file_size{0};                                  // 0 means disabled
  uint32_t _max_backup_files{(std::numeric_limits<uint32_t>::max)()}; // max means disabled
  uint32_t _rotation_interval{0};                                     // 0 means disabled
  RotationFrequency _rotation_frequency{RotationFrequency::Disabled};
  RotationNamingScheme _rotation_naming_scheme{RotationNamingScheme::Index};
  bool _overwrite_rolled_files{true};
  bool _remove_old_files{true};
  bool _rotation_on_creation{false};
};

/**
 * @brief The RotatingSink class
 */
template <typename TBase>
class RotatingSink : public TBase
{
public:
  using base_type = TBase;

  /**
   * @brief Constructor.
   *
   * Creates a new instance of the RotatingSink class.
   *
   * @param filename The base file name to be used for logs.
   * @param config The sink configuration.
   * @param file_event_notifier file event notifier
   * @param start_time start time
   */
  RotatingSink(fs::path const& filename, RotatingFileSinkConfig const& config,
               FileEventNotifier file_event_notifier = FileEventNotifier{},
               std::chrono::system_clock::time_point start_time = std::chrono::system_clock::now())
    : base_type(filename, static_cast<FileSinkConfig const&>(config),
                std::move(file_event_notifier), false, start_time),
      _config(config)
  {
    uint64_t const today_timestamp_ns = static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(start_time.time_since_epoch()).count());

    _clean_and_recover_files(this->_filename, _config.open_mode(), today_timestamp_ns);

    if (_config.rotation_frequency() != RotatingFileSinkConfig::RotationFrequency::Disabled)
    {
      // Calculate next rotation time
      _next_rotation_time = _calculate_initial_rotation_tp(
        static_cast<uint64_t>(
          std::chrono::duration_cast<std::chrono::nanoseconds>(start_time.time_since_epoch()).count()),
        config);
    }

    _open_file_timestamp = static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(start_time.time_since_epoch()).count());

    _created_files.emplace_front(this->_filename, 0, std::string{});

    // Rotate an existing startup file before the initial open so mode='w' does not truncate it.
    bool const should_rotate_on_creation =
      _config.rotation_on_creation() && !this->is_null() && (_get_file_size(this->_filename) > 0);
    bool const can_rotate_on_creation = should_rotate_on_creation &&
      ((_created_files.size() <= _config.max_backup_files()) || _config.overwrite_rolled_files());

    if (can_rotate_on_creation)
    {
      _rotate_closed_file(today_timestamp_ns);
    }
    else
    {
      // If startup rotation was requested but cannot proceed, preserve the existing file and
      // continue without truncating it.
      std::string const open_mode = should_rotate_on_creation
        ? _reopen_mode_after_failed_rotation(_config.open_mode())
        : _config.open_mode();
      this->open_file(this->_filename, open_mode);
    }

    if (!this->is_null())
    {
      this->_file_size = _get_file_size(this->_filename);
    }
  }

  ~RotatingSink() override = default;

  /**
   * @brief Writes a formatted log message to the stream
   * @param log_metadata The metadata of the log message
   * @param log_timestamp The timestamp of the log message
   * @param thread_id The ID of the thread that generated the log message
   * @param thread_name The name of the thread that generated the log message
   * @param process_id Process Id
   * @param logger_name logger name
   * @param log_level Log level of the message.
   * @param log_level_description Description of the log level.
   * @param log_level_short_code Short code representing the log level.
   * @param named_args Structured key-value pairs associated with the log message
   * @param log_message The log message to write
   * @param log_statement The full log statement
   */
  QUILL_ATTRIBUTE_HOT void write_log(MacroMetadata const* log_metadata, uint64_t log_timestamp,
                                     std::string_view thread_id, std::string_view thread_name,
                                     std::string const& process_id, std::string_view logger_name,
                                     LogLevel log_level, std::string_view log_level_description,
                                     std::string_view log_level_short_code,
                                     std::vector<std::pair<std::string, std::string>> const* named_args,
                                     std::string_view log_message, std::string_view log_statement) override
  {
    if (this->is_null())
    {
      base_type::write_log(log_metadata, log_timestamp, thread_id, thread_name, process_id,
                           logger_name, log_level, log_level_description, log_level_short_code,
                           named_args, log_message, log_statement);
      return;
    }

    bool time_rotation = false;

    if (_config.rotation_frequency() != RotatingFileSinkConfig::RotationFrequency::Disabled)
    {
      // Check if we need to rotate based on time
      time_rotation = _time_rotation(log_timestamp);
    }

    if (!time_rotation && _config.rotation_max_file_size() != 0)
    {
      size_t const write_size = this->estimate_write_size(
        log_metadata, log_timestamp, thread_id, thread_name, process_id, logger_name, log_level,
        log_level_description, log_level_short_code, named_args, log_message, log_statement);

      // Check if we need to rotate based on size
      QUILL_TRY { _size_rotation(write_size, log_timestamp); }
#if !defined(QUILL_NO_EXCEPTIONS)
      QUILL_CATCH_ALL()
      {
        // The estimate may have cached state for this record (e.g. JsonSink caches the built
        // json message). The write is abandoned, so discard it before propagating, otherwise a
        // later record could consume the stale cached state
        base_type::discard_write_estimate();
        throw;
      }
#endif
    }

    // write to file
    base_type::write_log(log_metadata, log_timestamp, thread_id, thread_name, process_id,
                         logger_name, log_level, log_level_description, log_level_short_code,
                         named_args, log_message, log_statement);
  }

protected:
  /**
   * Used in regression tests
   */
  virtual bool rename_file(fs::path const& previous_file, fs::path const& new_file) noexcept
  {
    return _rename_file(previous_file, new_file);
  }

private:
  /***/
  QUILL_NODISCARD bool _time_rotation(uint64_t record_timestamp_ns)
  {
    if (record_timestamp_ns >= _next_rotation_time)
    {
      _rotate_files(record_timestamp_ns);
      _next_rotation_time = _calculate_rotation_tp(_next_rotation_time, record_timestamp_ns, _config);
      return true;
    }

    return false;
  }

  /***/
  void _size_rotation(size_t log_msg_size, uint64_t record_timestamp_ns)
  {
    // Calculate the new size of the file
    if ((this->_file_size + log_msg_size) > _config.rotation_max_file_size())
    {
      _rotate_files(record_timestamp_ns);
    }
  }

  /***/
  void _rotate_files(uint64_t record_timestamp_ns)
  {
    if ((_created_files.size() > _config.max_backup_files()) && !_config.overwrite_rolled_files())
    {
      // We have reached the max number of backup files, and we are not allowed to overwrite the
      // oldest file. We will stop rotating
      return;
    }

    if (!this->is_open())
    {
      // A previous rotation left the sink without an open file (e.g. the disk was full when
      // reopen was attempted). Try to recover by reopening before doing anything else, so we
      // do not flush/fsync a null handle and re-enter this path again on the next write.
      this->open_file(this->_filename, _reopen_mode_after_failed_rotation(_config.open_mode()));
      _open_file_timestamp = record_timestamp_ns;
      this->_file_size = _get_file_size(this->_filename);
      return;
    }

    // We need to flush and also fsync before actually getting the size of the file
    base_type::flush_sink();
    base_type::fsync_file(true);

    if (_get_file_size(this->_filename) == 0)
    {
      // Also check the file size is > 0  to better deal with full disk
      return;
    }

    this->close_file();
    _rotate_closed_file(record_timestamp_ns);
  }

  /***/
  void _rotate_closed_file(uint64_t record_timestamp_ns)
  {
    bool rotation_succeeded = true;

    // datetime_suffix will be empty if we are using the default naming scheme
    std::string datetime_suffix;
    if (_config.rotation_naming_scheme() == RotatingFileSinkConfig::RotationNamingScheme::Date)
    {
      datetime_suffix =
        this->format_datetime_string(_open_file_timestamp, _config.timezone(), "%Y%m%d");
    }
    else if (_config.rotation_naming_scheme() == RotatingFileSinkConfig::RotationNamingScheme::DateAndTime)
    {
      datetime_suffix =
        this->format_datetime_string(_open_file_timestamp, _config.timezone(), "%Y%m%d_%H%M%S");
    }

    // We need to rotate the files and rename them with an index.
    // Track completed renames so we can undo them if a later rename fails.
    std::vector<RenameRecord> completed_renames;

    for (auto it = _created_files.rbegin(); it != _created_files.rend(); ++it)
    {
      // Create each existing filename on disk with the existing index.
      // when the index is 0 we want to rename the latest file
      fs::path existing_file;
      fs::path renamed_file;

      existing_file = _get_filename(it->base_filename, it->index, it->date_time);

      // increment the index if needed and rename the file
      uint32_t index_to_use = it->index;

      if (_config.rotation_naming_scheme() == RotatingFileSinkConfig::RotationNamingScheme::Index ||
          it->date_time == datetime_suffix)
      {
        // we are rotating and incrementing the index, or we have another file with the same date_time suffix
        uint32_t const new_index = index_to_use + 1;

        renamed_file = _get_filename(it->base_filename, new_index, datetime_suffix);

        if (!rename_file(existing_file, renamed_file))
        {
          rotation_succeeded = false;
          break;
        }

        size_t const idx = static_cast<size_t>(std::distance(_created_files.begin(), it.base()) - 1);
        completed_renames.push_back({existing_file, renamed_file, idx, new_index, datetime_suffix});
      }
      else if (it->date_time.empty())
      {
        // we are renaming the latest file
        uint32_t const new_index = index_to_use;

        renamed_file = _get_filename(it->base_filename, new_index, datetime_suffix);

        if (!rename_file(existing_file, renamed_file))
        {
          rotation_succeeded = false;
          break;
        }

        size_t const idx = static_cast<size_t>(std::distance(_created_files.begin(), it.base()) - 1);
        completed_renames.push_back({existing_file, renamed_file, idx, new_index, datetime_suffix});
      }
    }

    if (!rotation_succeeded)
    {
      // Undo completed renames in reverse order to restore on-disk state
      for (auto rit = completed_renames.rbegin(); rit != completed_renames.rend(); ++rit)
      {
        rename_file(rit->renamed, rit->original);
      }

      this->open_file(this->_filename, _reopen_mode_after_failed_rotation(_config.open_mode()));
      this->_file_size = _get_file_size(this->_filename);
      return;
    }

    // All renames succeeded — apply metadata updates
    for (auto const& rec : completed_renames)
    {
      _created_files[rec.created_files_idx].index = rec.new_index;
      _created_files[rec.created_files_idx].date_time = rec.new_date_time;
    }

    // Check if we have too many files in the queue remove_file the oldest one
    if (_created_files.size() > _config.max_backup_files())
    {
      // remove_file that file from the system and also pop it from the queue
      fs::path const removed_file = _get_filename(
        _created_files.back().base_filename, _created_files.back().index, _created_files.back().date_time);
      _remove_file(removed_file);
      _created_files.pop_back();
    }

    // add the current file back to the list with index 0
    _created_files.emplace_front(this->_filename, 0, std::string{});

    // Open file for logging
    this->open_file(this->_filename, _config.open_mode());
    _open_file_timestamp = record_timestamp_ns;
    this->_file_size = 0;
  }

  /***/
  void _clean_and_recover_files(fs::path const& filename, std::string const& open_mode, uint64_t today_timestamp_ns)
  {
    // The normalized filename is absolute in practice, so resolving against the current working
    // directory only matters for relative paths. fs::current_path() can throw when the working
    // directory has been deleted, so use the non-throwing overload and skip clean-up on failure.
    fs::path parent_dir = filename.parent_path();

    if (parent_dir.is_relative())
    {
      std::error_code current_path_ec;
      parent_dir = fs::current_path(current_path_ec) / parent_dir;

      if (current_path_ec)
      {
        return;
      }
    }

    {
      std::error_code ec;
      if (!fs::exists(parent_dir, ec) || ec)
      {
        // Directory does not exist yet; nothing to clean or recover.
        // open_file will create it later.
        return;
      }
    }

    // if we are starting in "w" mode, then we also should clean all previous log files of the previous run
    if (_config.remove_old_files() && (open_mode.find('w') != std::string::npos))
    {
      if (_config.rotation_naming_scheme() == RotatingFileSinkConfig::RotationNamingScheme::DateAndTime)
      {
        // DateAndTime filenames are unique across runs and cannot collide, nothing to clean
        return;
      }

      // Collect paths first, then remove — deleting during directory_iterator
      // traversal is implementation-defined and can skip entries on some platforms.
      std::vector<fs::path> files_to_remove;

      for (auto const& entry : fs::directory_iterator(parent_dir))
      {
        ParsedRotatedFileInfo parsed_file_info;
        if (!_try_parse_rotated_file(filename, entry.path(), parsed_file_info))
        {
          continue;
        }

        if (_config.rotation_naming_scheme() == RotatingFileSinkConfig::RotationNamingScheme::Index)
        {
          files_to_remove.push_back(entry.path());
        }
        else if (_config.rotation_naming_scheme() == RotatingFileSinkConfig::RotationNamingScheme::Date)
        {
          std::string const today_date =
            this->format_datetime_string(today_timestamp_ns, _config.timezone(), "%Y%m%d");

          if (parsed_file_info.date_time == today_date)
          {
            files_to_remove.push_back(entry.path());
          }
        }
      }

      for (auto const& file_path : files_to_remove)
      {
        fs::remove(file_path);
      }
    }
    else if (open_mode.find('a') != std::string::npos)
    {
      // we need to recover the rotated files of the previous runs, so that rotation continues
      // with the correct index and max_backup_files also counts files already on disk
      for (auto const& entry : fs::directory_iterator(parent_dir))
      {
        ParsedRotatedFileInfo parsed_file_info;
        if (!_try_parse_rotated_file(filename, entry.path(), parsed_file_info))
        {
          continue;
        }

        _created_files.emplace_front(filename, parsed_file_info.index, parsed_file_info.date_time);
      }

      // sort the recovered files with the newest first; within the same date_time suffix a
      // greater index means an older file
      std::sort(_created_files.begin(), _created_files.end(),
                [](FileInfo const& a, FileInfo const& b)
                {
                  if (a.date_time != b.date_time)
                  {
                    return a.date_time > b.date_time;
                  }
                  return a.index < b.index;
                });

      if (_config.overwrite_rolled_files())
      {
        // remove the oldest recovered files when they exceed max_backup_files, so that files
        // from previous runs do not keep accumulating across restarts
        while (_created_files.size() > _config.max_backup_files())
        {
          FileInfo const& oldest_file = _created_files.back();
          _remove_file(_get_filename(oldest_file.base_filename, oldest_file.index, oldest_file.date_time));
          _created_files.pop_back();
        }
      }
    }
  }

  /***/
  QUILL_NODISCARD static size_t _get_file_size(fs::path const& filename)
  {
    std::error_code ec;
    auto const size = fs::file_size(filename, ec);
    if (ec)
    {
      return 0;
    }
    return static_cast<size_t>(size);
  }

  struct ParsedRotatedFileInfo
  {
    uint32_t index{0};
    std::string date_time;
  };

  QUILL_NODISCARD static bool _parse_uint32(std::string_view value, uint32_t& parsed_value) noexcept
  {
    if (value.empty())
    {
      return false;
    }

    uint64_t parsed{0};
    for (char const c : value)
    {
      if ((c < '0') || (c > '9'))
      {
        return false;
      }

      parsed = (parsed * 10u) + static_cast<uint32_t>(c - '0');
      if (parsed > (std::numeric_limits<uint32_t>::max)())
      {
        return false;
      }
    }

    parsed_value = static_cast<uint32_t>(parsed);
    return true;
  }

  QUILL_NODISCARD bool _try_parse_rotated_file(fs::path const& base_filename, fs::path const& candidate_filename,
                                               ParsedRotatedFileInfo& parsed_file_info) const
  {
    std::string const base_extension = base_filename.extension().string();
    std::string const base_stem = base_filename.stem().string();
    std::string const candidate_name = candidate_filename.filename().string();
    std::string_view candidate_stem{candidate_name};

    if (!base_extension.empty())
    {
      if ((candidate_name.size() <= base_extension.size()) ||
          (candidate_name.compare(candidate_name.size() - base_extension.size(),
                                  base_extension.size(), base_extension) != 0))
      {
        return false;
      }

      candidate_stem =
        std::string_view{candidate_name.data(), candidate_name.size() - base_extension.size()};
    }

    if ((candidate_stem.size() <= (base_stem.size() + 1)) ||
        (candidate_stem.compare(0, base_stem.size(), base_stem) != 0) ||
        (candidate_stem[base_stem.size()] != '.'))
    {
      return false;
    }

    std::string_view const suffix{candidate_stem.data() + base_stem.size() + 1,
                                  candidate_stem.size() - base_stem.size() - 1};

    if (_config.rotation_naming_scheme() == RotatingFileSinkConfig::RotationNamingScheme::Index)
    {
      parsed_file_info.date_time.clear();
      return _parse_uint32(suffix, parsed_file_info.index);
    }

    if (_config.rotation_naming_scheme() == RotatingFileSinkConfig::RotationNamingScheme::Date)
    {
      size_t const separator_pos = suffix.find('.');
      std::string_view date_part =
        (separator_pos == std::string_view::npos) ? suffix : suffix.substr(0, separator_pos);

      if ((date_part.size() != 8u) || !_parse_uint32(date_part, parsed_file_info.index))
      {
        // With an extensionless base filename, indexed Date files are generated as
        // "name.<index>.<date>" because the date suffix is treated as the extension when the
        // index is appended.
        if (base_extension.empty() && (separator_pos != std::string_view::npos) &&
            (suffix.find('.', separator_pos + 1) == std::string_view::npos))
        {
          std::string_view const index_part = suffix.substr(0, separator_pos);
          date_part = suffix.substr(separator_pos + 1);

          uint32_t date_part_value{0};
          if (_parse_uint32(index_part, parsed_file_info.index) && (date_part.size() == 8u) &&
              _parse_uint32(date_part, date_part_value))
          {
            parsed_file_info.date_time.assign(date_part.data(), date_part.size());
            return true;
          }
        }

        return false;
      }

      parsed_file_info.date_time.assign(date_part.data(), date_part.size());
      parsed_file_info.index = 0;

      if (separator_pos == std::string_view::npos)
      {
        return true;
      }

      if (suffix.find('.', separator_pos + 1) != std::string_view::npos)
      {
        return false;
      }

      std::string_view const index_part = suffix.substr(separator_pos + 1);
      return _parse_uint32(index_part, parsed_file_info.index);
    }

    if (_config.rotation_naming_scheme() == RotatingFileSinkConfig::RotationNamingScheme::DateAndTime)
    {
      size_t const separator_pos = suffix.find('.');
      std::string_view date_time_part =
        (separator_pos == std::string_view::npos) ? suffix : suffix.substr(0, separator_pos);

      // expect the %Y%m%d_%H%M%S format
      uint32_t date_part_value{0};
      uint32_t time_part_value{0};
      if ((date_time_part.size() != 15u) || (date_time_part[8] != '_') ||
          !_parse_uint32(date_time_part.substr(0, 8), date_part_value) ||
          !_parse_uint32(date_time_part.substr(9), time_part_value))
      {
        // With an extensionless base filename, indexed DateAndTime files are generated as
        // "name.<index>.<date_time>" because the date_time suffix is treated as the extension
        // when the index is appended.
        if (base_extension.empty() && (separator_pos != std::string_view::npos) &&
            (suffix.find('.', separator_pos + 1) == std::string_view::npos))
        {
          std::string_view const index_part = suffix.substr(0, separator_pos);
          date_time_part = suffix.substr(separator_pos + 1);

          if (_parse_uint32(index_part, parsed_file_info.index) && (date_time_part.size() == 15u) &&
              (date_time_part[8] == '_') && _parse_uint32(date_time_part.substr(0, 8), date_part_value) &&
              _parse_uint32(date_time_part.substr(9), time_part_value))
          {
            parsed_file_info.date_time.assign(date_time_part.data(), date_time_part.size());
            return true;
          }
        }

        return false;
      }

      parsed_file_info.date_time.assign(date_time_part.data(), date_time_part.size());
      parsed_file_info.index = 0;

      if (separator_pos == std::string_view::npos)
      {
        return true;
      }

      if (suffix.find('.', separator_pos + 1) != std::string_view::npos)
      {
        return false;
      }

      std::string_view const index_part = suffix.substr(separator_pos + 1);
      return _parse_uint32(index_part, parsed_file_info.index);
    }

    return false;
  }

  /***/
  static void _remove_file(fs::path const& filename) noexcept
  {
    std::error_code ec;

    fs::file_status const status = fs::status(filename, ec);

    if (ec || status.type() != fs::file_type::regular)
    {
      // File doesn't exist or is not a regular file
      return;
    }

    fs::remove(filename, ec);
  }

  /***/
  QUILL_NODISCARD static std::string _reopen_mode_after_failed_rotation(std::string const& open_mode)
  {
    std::string reopen_mode = open_mode;

    if (!reopen_mode.empty() && reopen_mode[0] == 'w')
    {
      reopen_mode[0] = 'a';
    }

    return reopen_mode;
  }

  /***/
  bool static _rename_file(fs::path const& previous_file, fs::path const& new_file) noexcept
  {
    std::error_code ec;
    fs::rename(previous_file, new_file, ec);

    if (ec)
    {
      // Retry once after a delay - workaround for Windows antivirus locking files
      // This is a common issue where antivirus software temporarily locks files during scanning
      detail::sleep_for_ns(250ull * 1'000'000ull); // 250 ms

      ec.clear();
      fs::rename(previous_file, new_file, ec);

      if (ec)
      {
        return false;
      }
    }

    return true;
  }

  /***/
  QUILL_NODISCARD static fs::path _append_index_to_filename(fs::path const& filename, uint32_t index)
  {
    if (index == 0u)
    {
      return filename;
    }

    // Get base file and extension
    auto const [stem, ext] = base_type::extract_stem_and_extension(filename);
    return fs::path{stem + "." + std::to_string(index) + ext};
  }

  /***/
  QUILL_NODISCARD static fs::path _append_string_to_filename(fs::path const& filename, std::string const& text)
  {
    if (text.empty())
    {
      return filename;
    }

    // Get base file and extension
    auto const [stem, ext] = base_type::extract_stem_and_extension(filename);
    return fs::path{stem + "." + text + ext};
  }

  /***/
  QUILL_NODISCARD static time_t _timestamp_ns_to_time_t(uint64_t timestamp_ns) noexcept
  {
// time_t on i386 is 32 bits so casting out of range number results in zero
#if (defined(__i386))
    return static_cast<time_t>(timestamp_ns / 1000000000);
#else
    return static_cast<time_t>(timestamp_ns) / 1000000000;
#endif
  }

  /***/
  QUILL_NODISCARD static uint64_t _calculate_next_daily_rotation_tp(time_t reference_time,
                                                                    RotatingFileSinkConfig const& config)
  {
    tm date;

    if (config.timezone() == Timezone::GmtTime)
    {
      detail::gmtime_rs(&reference_time, &date);
    }
    else
    {
      detail::localtime_rs(&reference_time, &date);
    }

    date.tm_hour = static_cast<decltype(date.tm_hour)>(config.daily_rotation_time().first.count());
    date.tm_min = static_cast<decltype(date.tm_min)>(config.daily_rotation_time().second.count());
    date.tm_sec = 0;

    auto to_timestamp = [&config](tm& rotation_time_tm)
    {
      if (config.timezone() == Timezone::LocalTime)
      {
        // Let mktime resolve the correct DST state for the configured wall-clock time.
        rotation_time_tm.tm_isdst = -1;
        return std::mktime(&rotation_time_tm);
      }

      return detail::timegm(&rotation_time_tm);
    };

    time_t rotation_time = to_timestamp(date);

    if (rotation_time <= reference_time)
    {
      date.tm_mday += 1;
      rotation_time = to_timestamp(date);
    }

    return static_cast<uint64_t>(std::chrono::nanoseconds{std::chrono::seconds{rotation_time}}.count());
  }

  /***/
  static uint64_t _calculate_initial_rotation_tp(uint64_t start_time_ns, RotatingFileSinkConfig const& config)
  {
    time_t const time_now = _timestamp_ns_to_time_t(start_time_ns);
    tm date;

    // here we do this because of `daily_rotation_time_str` that might have specified the time in UTC
    if (config.timezone() == Timezone::GmtTime)
    {
      detail::gmtime_rs(&time_now, &date);
    }
    else
    {
      detail::localtime_rs(&time_now, &date);
    }

    // update to the desired date
    if (config.rotation_frequency() == RotatingFileSinkConfig::RotationFrequency::Minutely)
    {
      date.tm_min += static_cast<decltype(date.tm_min)>(config.rotation_interval());
      date.tm_sec = 0;
    }
    else if (config.rotation_frequency() == RotatingFileSinkConfig::RotationFrequency::Hourly)
    {
      date.tm_hour += static_cast<decltype(date.tm_hour)>(config.rotation_interval());
      date.tm_min = 0;
      date.tm_sec = 0;
    }
    else if (config.rotation_frequency() == RotatingFileSinkConfig::RotationFrequency::Daily)
    {
      return _calculate_next_daily_rotation_tp(time_now, config);
    }
    else
    {
      QUILL_THROW(QuillError{"Invalid rotation frequency"});
    }

    // convert back to timestamp
    time_t const rotation_time =
      (config.timezone() == Timezone::GmtTime) ? detail::timegm(&date) : std::mktime(&date);

    auto const interval_seconds =
      (config.rotation_frequency() == RotatingFileSinkConfig::RotationFrequency::Minutely)
      ? std::chrono::duration_cast<std::chrono::seconds>(std::chrono::minutes{config.rotation_interval()})
          .count()
      : std::chrono::duration_cast<std::chrono::seconds>(std::chrono::hours{config.rotation_interval()})
          .count();

    uint64_t const rotation_time_seconds = (rotation_time > time_now)
      ? static_cast<uint64_t>(rotation_time)
      : static_cast<uint64_t>(rotation_time + interval_seconds);

    return static_cast<uint64_t>(
      std::chrono::nanoseconds{std::chrono::seconds{rotation_time_seconds}}.count());
  }

  /***/
  static uint64_t _calculate_rotation_tp(uint64_t previous_rotation_tp, uint64_t record_timestamp_ns,
                                         RotatingFileSinkConfig const& config)
  {
    if ((config.rotation_frequency() == RotatingFileSinkConfig::RotationFrequency::Minutely) ||
        (config.rotation_frequency() == RotatingFileSinkConfig::RotationFrequency::Hourly))
    {
      uint64_t const interval_ns =
        (config.rotation_frequency() == RotatingFileSinkConfig::RotationFrequency::Minutely)
        ? static_cast<uint64_t>(
            std::chrono::nanoseconds{std::chrono::minutes{config.rotation_interval()}}.count())
        : static_cast<uint64_t>(
            std::chrono::nanoseconds{std::chrono::hours{config.rotation_interval()}}.count());

      // Advance from the previous boundary in whole intervals so that a late-triggered rotation
      // (e.g. sparse logging past a missed boundary) does not permanently shift the following
      // boundaries away from the wall-clock alignment of the initial rotation time point
      uint64_t const intervals_elapsed = (record_timestamp_ns - previous_rotation_tp) / interval_ns;
      return previous_rotation_tp + ((intervals_elapsed + 1u) * interval_ns);
    }

    if (config.rotation_frequency() == RotatingFileSinkConfig::RotationFrequency::Daily)
    {
      return _calculate_next_daily_rotation_tp(_timestamp_ns_to_time_t(record_timestamp_ns), config);
    }

    QUILL_THROW(QuillError{"Invalid rotation frequency"});
  }

  /***/
  static fs::path _get_filename(fs::path filename, uint32_t index, std::string const& date_time)
  {
    if (!date_time.empty())
    {
      filename = _append_string_to_filename(filename, date_time);
    }

    if (index > 0)
    {
      filename = _append_index_to_filename(filename, index);
    }

    return filename;
  }

protected:
  struct FileInfo
  {
    FileInfo(fs::path base_filename, uint32_t index, std::string date_time)
      : base_filename{std::move(base_filename)}, date_time{std::move(date_time)}, index{index}
    {
    }

    fs::path base_filename;
    std::string date_time;
    uint32_t index;
  };

  struct RenameRecord
  {
    fs::path original;
    fs::path renamed;
    size_t created_files_idx;
    uint32_t new_index;
    std::string new_date_time;
  };

  std::deque<FileInfo> _created_files; /**< We store in a queue the filenames we created, first: index, second: date/datetime, third: base_filename */
  uint64_t _next_rotation_time{0};     /**< The next rotation time point */
  uint64_t _open_file_timestamp{0};    /**< The timestamp of the currently open file */
  RotatingFileSinkConfig _config;
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
