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

#ifndef QUILL_MODULE
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
#endif

#if !defined(_WIN32)
  #include <sys/stat.h>
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

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
  RotatingFileSinkConfig() = default;

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
    RotationFrequency rotation_frequency;
    uint64_t interval_unit_ns;

    if (frequency == 'M' || frequency == 'm')
    {
      rotation_frequency = RotationFrequency::Minutely;
      interval_unit_ns = 60ull * 1'000'000'000ull;
    }
    else if (frequency == 'H' || frequency == 'h')
    {
      rotation_frequency = RotationFrequency::Hourly;
      interval_unit_ns = 60ull * 60ull * 1'000'000'000ull;
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

    if (static_cast<uint64_t>(interval) >
        (static_cast<uint64_t>((std::numeric_limits<int64_t>::max)()) / interval_unit_ns))
    {
      QUILL_THROW(QuillError{"rotation interval is too large to represent in nanoseconds"});
    }

    _rotation_frequency = rotation_frequency;
    _rotation_interval = interval;
    _daily_rotation_time = {};
  }

  /**
   * @brief Sets the time of day for daily log file rotation.
   * @param daily_rotation_time_str The time of day for rotation (format: "HH:MM")
   */
  QUILL_ATTRIBUTE_COLD void set_rotation_time_daily(std::string const& daily_rotation_time_str)
  {
    _daily_rotation_time = _parse_daily_rotation_time(daily_rotation_time_str);
    _rotation_frequency = RotationFrequency::Daily;
    _rotation_interval = 0;
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
   * count is reached, and log file rotation will stop until a backup file is removed. The default
   * value is true.
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
  static std::pair<std::chrono::hours, std::chrono::minutes> _parse_daily_rotation_time(std::string const& daily_rotation_time_str)
  {
    if ((daily_rotation_time_str.size() != 5) || (daily_rotation_time_str[2] != ':'))
    {
      QUILL_THROW(
        QuillError{"Invalid daily_rotation_time_str value format. The format should be `HH:MM`."});
    }

    std::chrono::hours const hours{_parse_two_digit_time_component(daily_rotation_time_str, 0)};
    std::chrono::minutes const minutes{_parse_two_digit_time_component(daily_rotation_time_str, 3)};

    if ((hours > std::chrono::hours{23}) || (minutes > std::chrono::minutes{59}))
    {
      QUILL_THROW(
        QuillError("Invalid rotation values. The hour value should be between 00 and 23, and the "
                   "minute value should be between 00 and 59."));
    }

    return std::make_pair(hours, minutes);
  }

  /** Parses the two digits at `pos` */
  static uint32_t _parse_two_digit_time_component(std::string const& time_str, size_t pos)
  {
    char const tens = time_str[pos];
    char const ones = time_str[pos + 1];

    if ((tens < '0') || (tens > '9') || (ones < '0') || (ones > '9'))
    {
      QUILL_THROW(QuillError{
        "Invalid daily_rotation_time_str value format. Each component of the time (HH and MM) "
        "should be two digits."});
    }

    return (static_cast<uint32_t>(tens - '0') * 10u) + static_cast<uint32_t>(ones - '0');
  }

private:
  std::pair<std::chrono::hours, std::chrono::minutes> _daily_rotation_time{}; // used only by Daily rotation
  size_t _rotation_max_file_size{0};                                          // 0 means disabled
  uint32_t _max_backup_files{(std::numeric_limits<uint32_t>::max)()};         // max means disabled
  uint32_t _rotation_interval{0};                                             // 0 means disabled
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
#if defined(_WIN32)
    // Windows has no /dev/null device. FileSink discards the output without opening a file, so
    // there is nothing to rotate. POSIX opens the real device below and only skips its rotation.
    if (this->is_null())
    {
      return;
    }
#endif

    uint64_t const start_time_ns = static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(start_time.time_since_epoch()).count());

    bool const recovery_succeeded =
      _clean_and_recover_files(this->_filename, _config.open_mode(), start_time_ns);

    if (recovery_succeeded && (_config.rotation_frequency() != RotatingFileSinkConfig::RotationFrequency::Disabled))
    {
      // Calculate next rotation time
      _next_rotation_time = _calculate_initial_rotation_tp(start_time_ns, config);
    }

    _open_file_timestamp = start_time_ns;

    if (recovery_succeeded)
    {
      _created_files.emplace_front(this->_filename, 0, std::string{});
    }

    // Rotate an existing startup file before the initial open so mode='w' does not truncate it.
    bool const rotation_on_creation_requested =
      _config.rotation_on_creation() && !this->is_null() && (_get_file_size(this->_filename) > 0);
    bool const can_rotate_on_creation = _rotation_enabled() && rotation_on_creation_requested &&
      ((_created_files.size() <= _config.max_backup_files()) || _config.overwrite_rolled_files());

    if (can_rotate_on_creation)
    {
      _rotate_closed_file(start_time_ns);
    }
    else
    {
      // If startup rotation was requested but cannot proceed, preserve the existing file and
      // continue without truncating it.
      std::string const open_mode = rotation_on_creation_requested
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
   * @brief Writes a log record, rotating the file first if needed.
   * @param record the log record
   */
  QUILL_ATTRIBUTE_HOT void write_log(LogRecord const& record) override
  {
    if (this->is_null() || !_rotation_enabled())
    {
      base_type::write_log(record);
      return;
    }

    if (QUILL_UNLIKELY(!this->is_open()))
    {
      // A failed reopen left no active file. Recover now, through the throttled retry in
      // _rotate_files(), rather than only when a record reaches a rotation threshold: records
      // written to the closed file are lost and never count towards it.
      _rotate_files(record.log_timestamp);
    }

    bool time_rotation = false;

    if (_config.rotation_frequency() != RotatingFileSinkConfig::RotationFrequency::Disabled)
    {
      // Check if we need to rotate based on time
      time_rotation = _time_rotation(record.log_timestamp);
    }

    if (!time_rotation && _config.rotation_max_file_size() != 0)
    {
      size_t const write_size = this->estimate_write_size(record);

      // Check if we need to rotate based on size
      QUILL_TRY { _size_rotation(write_size, record.log_timestamp); }
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

    if (QUILL_UNLIKELY(this->_file_size == 0))
    {
      // The file holds no records yet, so name its backup after this record, as rotation does for
      // every later file, rather than after when the sink was created. A sparse logger's first
      // record can arrive days later.
      _open_file_timestamp = record.log_timestamp;
    }

    // write to file
    base_type::write_log(record);
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
  QUILL_NODISCARD bool _rotation_enabled() const noexcept { return !_created_files.empty(); }

  /***/
  QUILL_NODISCARD bool _time_rotation(uint64_t record_timestamp_ns)
  {
    if (record_timestamp_ns >= _next_rotation_time)
    {
      // A failed rotation stays due, so it is retried after rotation_retry_interval instead of at
      // the next scheduled rotation
      if (_rotate_files(record_timestamp_ns))
      {
        _next_rotation_time = _calculate_rotation_tp(_next_rotation_time, record_timestamp_ns, _config);
      }

      return true;
    }

    return false;
  }

  /***/
  void _size_rotation(size_t log_msg_size, uint64_t record_timestamp_ns)
  {
    uint64_t const max_file_size = _config.rotation_max_file_size();
    if ((this->_file_size > max_file_size) || (log_msg_size > (max_file_size - this->_file_size)))
    {
      _rotate_files(record_timestamp_ns);
    }
  }

  /**
   * @return false if the file was not rotated and the rotation should be retried, e.g. a rename
   * failed or a failed rotation waits for its retry interval
   */
  bool _rotate_files(uint64_t record_timestamp_ns)
  {
    // A failed rotation is retried once per interval rather than for every record, because each
    // attempt flushes, closes and reopens the file.
    auto const now = std::chrono::steady_clock::now();
    if (now < _next_rotation_attempt)
    {
      return false;
    }

    if (!this->is_open())
    {
      // A previous rotation left the sink without an open file (e.g. the disk was full when
      // reopen was attempted). Try to recover by reopening before doing anything else, so we
      // do not flush/fsync a null handle and re-enter this path again on the next write.
      _next_rotation_attempt = now + rotation_retry_interval;
      this->open_file(this->_filename, _reopen_mode_after_failed_rotation(_config.open_mode()));
      _next_rotation_attempt = {};
      _open_file_timestamp = record_timestamp_ns;
      this->_file_size = _get_file_size(this->_filename);
      return false;
    }

    // Checked after the recovery above: a sink that may not create another backup must still
    // reopen its active file
    if ((_created_files.size() > _config.max_backup_files()) && !_config.overwrite_rolled_files())
    {
      // We have reached the max number of backup files, and we are not allowed to overwrite the
      // oldest file. Rotate again only once a backup was removed outside Quill. That check touches
      // the disk, so run it at most once per retry interval rather than for every record.
      if (now < _next_backup_limit_check)
      {
        return true;
      }

      _next_backup_limit_check = now + rotation_retry_interval;
      _forget_missing_backups();

      if (_created_files.size() > _config.max_backup_files())
      {
        return true;
      }
    }

    base_type::flush_sink(SinkFlushReason::Periodic);

    if (_config.fsync_enabled())
    {
      // The rotated file is closed next, so fsync it now regardless of minimum_fsync_interval
      base_type::fsync_file(true);
    }

    if (_get_open_file_size() == 0)
    {
      // Also check the file size is > 0  to better deal with full disk. The file holds no records
      // yet, so date the next backup from now rather than from when it was opened.
      _open_file_timestamp = record_timestamp_ns;
      return true;
    }

    this->close_file();
    return _rotate_closed_file(record_timestamp_ns);
  }

  /** @return false if a rename failed and the file was reopened without rotating */
  bool _rotate_closed_file(uint64_t record_timestamp_ns)
  {
    bool rotation_succeeded = true;

    // datetime_suffix will be empty if we are using the default naming scheme
    std::string datetime_suffix;
    if (_config.rotation_naming_scheme() == RotatingFileSinkConfig::RotationNamingScheme::Date)
    {
      datetime_suffix =
        detail::format_datetime_string(_open_file_timestamp, _config.timezone(), "%Y%m%d");
    }
    else if (_config.rotation_naming_scheme() == RotatingFileSinkConfig::RotationNamingScheme::DateAndTime)
    {
      datetime_suffix =
        detail::format_datetime_string(_open_file_timestamp, _config.timezone(), "%Y%m%d_%H%M%S");
    }

    // Renaming a missing backup would fail and abort this and every later rotation. Without a
    // backup limit the number of tracked backups is unbounded, and a rotation that names backups
    // by date only renames those with its own suffix, so only those are checked on disk.
    bool const checks_own_suffix_only =
      (_config.rotation_naming_scheme() != RotatingFileSinkConfig::RotationNamingScheme::Index) &&
      (_config.max_backup_files() == (std::numeric_limits<uint32_t>::max)());

    _forget_missing_backups(checks_own_suffix_only ? &datetime_suffix : nullptr);

    if (_config.rotation_naming_scheme() == RotatingFileSinkConfig::RotationNamingScheme::DateAndTime)
    {
      _track_backups_with_date_time(datetime_suffix);

      if ((_created_files.size() > _config.max_backup_files()) && !_config.overwrite_rolled_files())
      {
        // The backups just tracked reach the backup limit, and rotating would remove the oldest of
        // them. Keep writing to the current file, as rotation does at the limit.
        this->open_file(this->_filename, _reopen_mode_after_failed_rotation(_config.open_mode()));
        this->_file_size = _get_file_size(this->_filename);
        return true;
      }
    }

    // A backup at the greatest index cannot take the next one: one more would name a file that
    // the sink does not write, and the one after that wraps to the active file. It stays where it
    // is, and so does the backup below it, which would be renamed over it, and so on down the run
    // of backups that ends at the greatest index. They stay tracked, as the oldest backups.
    bool const renames_every_backup =
      _config.rotation_naming_scheme() == RotatingFileSinkConfig::RotationNamingScheme::Index;

    auto const takes_next_index = [renames_every_backup, &datetime_suffix](FileInfo const& file_info)
    { return renames_every_backup || (file_info.date_time == datetime_suffix); };

    uint32_t first_kept_index = max_backup_index + 1u;

    for (uint32_t index = max_backup_index; index > 0; --index)
    {
      bool const is_tracked =
        std::any_of(_created_files.begin(), _created_files.end(),
                    [&takes_next_index, index](FileInfo const& file_info)
                    { return takes_next_index(file_info) && (file_info.index == index); });

      if (!is_tracked)
      {
        break;
      }

      first_kept_index = index;
    }

    // We need to rotate the files and rename them with an index.
    // Track completed renames so we can undo them if a later rename fails.
    std::vector<RenameRecord> completed_renames;

    for (auto it = _created_files.rbegin(); it != _created_files.rend(); ++it)
    {
      if (takes_next_index(*it) && (it->index >= first_kept_index))
      {
        // One of the backups that stay where they are
        continue;
      }

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

      _next_rotation_attempt = std::chrono::steady_clock::now() + rotation_retry_interval;
      this->open_file(this->_filename, _reopen_mode_after_failed_rotation(_config.open_mode()));
      this->_file_size = _get_file_size(this->_filename);
      return false;
    }

    // All renames succeeded — apply metadata updates
    for (auto const& rec : completed_renames)
    {
      _created_files[rec.created_files_idx].index = rec.new_index;
      _created_files[rec.created_files_idx].date_time = rec.new_date_time;
    }

    // Remove the oldest files while there are too many. It is more than one when the backups of
    // an earlier run were tracked by this rotation, see _track_backups_with_date_time()
    while (_created_files.size() > _config.max_backup_files())
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
    return true;
  }

  /***/
  QUILL_NODISCARD bool _clean_and_recover_files(fs::path const& filename, std::string const& open_mode,
                                                uint64_t today_timestamp_ns)
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
        return false;
      }
    }

    {
      std::error_code ec;
      bool const parent_exists = fs::exists(parent_dir, ec);
      if (ec)
      {
        return false;
      }

      if (!parent_exists)
      {
        // Directory does not exist yet; nothing to clean or recover.
        // open_file will create it later.
        return true;
      }
    }

    // if we are starting in "w" mode, then we also should clean all previous log files of the previous run
    if (_config.remove_old_files() && (open_mode.find('w') != std::string::npos))
    {
      if (_config.rotation_naming_scheme() == RotatingFileSinkConfig::RotationNamingScheme::DateAndTime)
      {
        // DateAndTime backups of previous runs are kept. A run that started in the same second
        // produces the same names, which _track_backups_with_date_time() handles on rotation
        return true;
      }

      // Collect paths first, then remove — deleting during directory_iterator
      // traversal is implementation-defined and can skip entries on some platforms.
      std::vector<fs::path> files_to_remove;

      // Iterate with the non-throwing overloads: under QUILL_NO_EXCEPTIONS a filesystem_error
      // thrown from construction or increment would be uncatchable and terminate at startup
      std::error_code dir_ec;
      for (fs::directory_iterator dir_it{parent_dir, dir_ec}, dir_end;
           !dir_ec && (dir_it != dir_end); dir_it.increment(dir_ec))
      {
        auto const& entry = *dir_it;

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
            detail::format_datetime_string(today_timestamp_ns, _config.timezone(), "%Y%m%d");

          if (parsed_file_info.date_time == today_date)
          {
            files_to_remove.push_back(entry.path());
          }
        }
      }

      if (dir_ec)
      {
        // Never remove a partially enumerated set. Unknown files could then be overwritten by a
        // later rotation, so disable rotation for this sink instance instead.
        return false;
      }

      for (auto const& file_path : files_to_remove)
      {
        _remove_file(file_path);
      }
    }
    else if (open_mode.find('a') != std::string::npos)
    {
      // we need to recover the rotated files of the previous runs, so that rotation continues
      // with the correct index and max_backup_files also counts files already on disk
      std::deque<FileInfo> recovered_files;
      std::error_code dir_ec;
      for (fs::directory_iterator dir_it{parent_dir, dir_ec}, dir_end;
           !dir_ec && (dir_it != dir_end); dir_it.increment(dir_ec))
      {
        auto const& entry = *dir_it;

        ParsedRotatedFileInfo parsed_file_info;
        if (!_try_parse_rotated_file(filename, entry.path(), parsed_file_info))
        {
          continue;
        }

        // Only a file is a backup: a directory with the name of one must not be renamed. Checked
        // after the name, so that the unrelated entries of the directory are not looked up. The
        // directory entry of the experimental filesystem has no is_regular_file().
        std::error_code status_ec;
        fs::file_status const entry_status = entry.status(status_ec);

        if (status_ec || !fs::is_regular_file(entry_status))
        {
          continue;
        }

        recovered_files.emplace_front(filename, parsed_file_info.index, parsed_file_info.date_time);
      }

      if (dir_ec)
      {
        // A partial view cannot safely drive index rotation or retention pruning.
        return false;
      }

      // sort the recovered files with the newest first; within the same date_time suffix a
      // greater index means an older file
      std::sort(recovered_files.begin(), recovered_files.end(),
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
        while (recovered_files.size() > _config.max_backup_files())
        {
          FileInfo const& oldest_file = recovered_files.back();
          _remove_file(_get_filename(oldest_file.base_filename, oldest_file.index, oldest_file.date_time));
          recovered_files.pop_back();
        }
      }

      _created_files = std::move(recovered_files);
    }

    return true;
  }

  /** Size of the open file from its handle, which, unlike the path, needs no fsync to be current. */
  QUILL_NODISCARD uint64_t _get_open_file_size() const noexcept
  {
#if defined(_WIN32)
    LARGE_INTEGER size{};
    return ::GetFileSizeEx(this->_native_file_handle, &size) ? static_cast<uint64_t>(size.QuadPart) : 0;
#else
    struct stat file_stat
    {
    };
    return (::fstat(fileno(this->_file), &file_stat) == 0) ? static_cast<uint64_t>(file_stat.st_size) : 0;
#endif
  }

  /**
   * Forgets backups that were removed or compressed outside Quill
   * @param date_time when not nullptr, only the backups named with this date and time are checked
   */
  void _forget_missing_backups(std::string const* date_time = nullptr)
  {
    _created_files.erase(
      std::remove_if(_created_files.begin(), _created_files.end(),
                     [date_time](FileInfo const& file_info)
                     {
                       if ((file_info.index == 0) && file_info.date_time.empty())
                       {
                         // the file being rotated
                         return false;
                       }

                       if (date_time && (file_info.date_time != *date_time))
                       {
                         return false;
                       }

                       std::error_code ec;
                       bool const exists = fs::exists(
                         _get_filename(file_info.base_filename, file_info.index, file_info.date_time), ec);
                       return !ec && !exists;
                     }),
      _created_files.end());
  }

  /***/
  QUILL_NODISCARD static uint64_t _get_file_size(fs::path const& filename)
  {
    std::error_code ec;
    uint64_t const size = fs::file_size(filename, ec);
    return ec ? 0 : size;
  }

  struct ParsedRotatedFileInfo
  {
    uint32_t index{0};
    std::string date_time;
  };

  using NativeFilenameView = std::basic_string_view<fs::path::value_type>;

  /** The greatest index of a backup, so that the index of a rotated backup never wraps to zero */
  static constexpr uint32_t max_backup_index{(std::numeric_limits<uint32_t>::max)() - 1u};

  // Date suffixes contain only validated ASCII digits and an optional underscore.
  static void _assign_date_suffix(std::string& result, NativeFilenameView suffix)
  {
    result.resize(suffix.size());
    std::transform(suffix.begin(), suffix.end(), result.begin(),
                   [](auto c) { return static_cast<char>(c); });
  }

  QUILL_NODISCARD static bool _parse_uint32(NativeFilenameView value, uint32_t& parsed_value) noexcept
  {
    if (value.empty())
    {
      return false;
    }

    uint64_t parsed{0};
    for (auto const c : value)
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

  /**
   * Parses the index of a backup filename as the sink writes it: a number from 1 to
   * max_backup_index, without leading zeros. The newest backup has no index in its name, so e.g.
   * "app.0.log" and "app.01.log" are not backups of "app.log".
   */
  QUILL_NODISCARD static bool _parse_index(NativeFilenameView value, uint32_t& parsed_value) noexcept
  {
    if (value.empty() || (value.front() == '0'))
    {
      return false;
    }

    return _parse_uint32(value, parsed_value) && (parsed_value <= max_backup_index);
  }

  /**
   * False when two filenames cannot name the same file: both are ASCII and differ other than by
   * case. Names with non-ASCII characters may differ by Unicode normalization, so they pass.
   */
  QUILL_NODISCARD static bool _may_be_filename_alias(fs::path::string_type const& lhs,
                                                     fs::path::string_type const& rhs) noexcept
  {
    auto const is_ascii = [](fs::path::string_type const& name)
    {
      for (auto const c : name)
      {
        if ((static_cast<uint32_t>(c) & ~0x7Fu) != 0)
        {
          return false;
        }
      }
      return true;
    };

    if (!is_ascii(lhs) || !is_ascii(rhs))
    {
      return true;
    }

    if (lhs.size() != rhs.size())
    {
      return false;
    }

    for (size_t i = 0; i < lhs.size(); ++i)
    {
      auto const to_lower = [](uint32_t c)
      { return ((c >= 'A') && (c <= 'Z')) ? (c + ('a' - 'A')) : c; };
      if (to_lower(static_cast<uint32_t>(lhs[i])) != to_lower(static_cast<uint32_t>(rhs[i])))
      {
        return false;
      }
    }

    return true;
  }

  QUILL_NODISCARD bool _try_parse_rotated_file(fs::path const& base_filename, fs::path const& candidate_filename,
                                               ParsedRotatedFileInfo& parsed_file_info) const
  {
    fs::path::string_type const base_extension = base_filename.extension().native();
    fs::path::string_type const base_stem = base_filename.stem().native();
    fs::path::string_type candidate_name = candidate_filename.filename().native();

    // Match "<stem>.<suffix><extension>", e.g. "app.1.log". The index/date is parsed below.
    // Require a non-empty suffix before indexing the separator or comparing the extension.
    bool const has_room_for_suffix = candidate_name.size() > base_stem.size() + base_extension.size() + 1u;
    bool exact_name_match{false};
    if (has_room_for_suffix)
    {
      bool const stem_matches = candidate_name.compare(0, base_stem.size(), base_stem) == 0;
      bool const separator_matches = candidate_name[base_stem.size()] == '.';
      bool const extension_matches = base_extension.empty() ||
        (candidate_name.compare(candidate_name.size() - base_extension.size(),
                                base_extension.size(), base_extension) == 0);

      exact_name_match = stem_matches && separator_matches && extension_matches;
    }

    if (!exact_name_match)
    {
      // Let the filesystem resolve case/normalization aliases. Do not mistake an unrelated
      // symlink to a backup for another backup, or collapse distinct hard-link names.
      // A directory can hold many unrelated files, so test the cheap condition first: the name
      // spelled with the base stem must exist. Only then compare inodes and canonical paths.
      fs::path::string_type const candidate_stem_and_suffix =
        base_extension.empty() ? candidate_name : candidate_filename.stem().native();
      bool candidate_is_regular_file{false};
      bool candidate_checked{false};

      for (size_t separator = candidate_stem_and_suffix.find('.'); separator != fs::path::string_type::npos;
           separator = candidate_stem_and_suffix.find('.', separator + 1u))
      {
        fs::path const alias = base_filename.parent_path() /
          (base_stem + candidate_stem_and_suffix.substr(separator) + base_extension);

        // Aliases differ only by case or Unicode normalization, so ASCII names must match
        // case-insensitively. This skips unrelated files without touching the filesystem.
        if (!_may_be_filename_alias(candidate_name, alias.filename().native()))
        {
          continue;
        }

        std::error_code ec;
        fs::file_status const alias_status = fs::symlink_status(alias, ec);
        if (ec || (alias_status.type() != fs::file_type::regular))
        {
          continue;
        }

        if (!candidate_checked)
        {
          candidate_checked = true;
          fs::file_status const status = fs::symlink_status(candidate_filename, ec);
          candidate_is_regular_file = !ec && (status.type() == fs::file_type::regular);
        }

        if (!candidate_is_regular_file || !fs::equivalent(candidate_filename, alias, ec) || ec)
        {
          continue;
        }

        // Same inode: an alias spelling of the same name, unless it is another hard link
        fs::path const canonical_candidate = detail::get_canonical_path(candidate_filename);
        if (!canonical_candidate.empty() && (detail::get_canonical_path(alias) == canonical_candidate))
        {
          candidate_name = alias.filename().native();
          break;
        }
      }
    }

    NativeFilenameView candidate_stem{candidate_name};

    if (!base_extension.empty())
    {
      if ((candidate_name.size() <= base_extension.size()) ||
          (candidate_name.compare(candidate_name.size() - base_extension.size(),
                                  base_extension.size(), base_extension) != 0))
      {
        return false;
      }

      candidate_stem =
        NativeFilenameView{candidate_name.data(), candidate_name.size() - base_extension.size()};
    }

    if ((candidate_stem.size() <= (base_stem.size() + 1)) ||
        (candidate_stem.compare(0, base_stem.size(), base_stem) != 0) ||
        (candidate_stem[base_stem.size()] != '.'))
    {
      return false;
    }

    NativeFilenameView const suffix{candidate_stem.data() + base_stem.size() + 1,
                                    candidate_stem.size() - base_stem.size() - 1};

    if (_config.rotation_naming_scheme() == RotatingFileSinkConfig::RotationNamingScheme::Index)
    {
      parsed_file_info.date_time.clear();
      return _parse_index(suffix, parsed_file_info.index);
    }

    if (_config.rotation_naming_scheme() == RotatingFileSinkConfig::RotationNamingScheme::Date)
    {
      size_t const separator_pos = suffix.find('.');
      NativeFilenameView date_part =
        (separator_pos == NativeFilenameView::npos) ? suffix : suffix.substr(0, separator_pos);

      if ((date_part.size() != 8u) || !_parse_uint32(date_part, parsed_file_info.index))
      {
        // With an extensionless base filename, indexed Date files are generated as
        // "name.<index>.<date>" because the date suffix is treated as the extension when the
        // index is appended.
        if (base_extension.empty() && (separator_pos != NativeFilenameView::npos) &&
            (suffix.find('.', separator_pos + 1) == NativeFilenameView::npos))
        {
          NativeFilenameView const index_part = suffix.substr(0, separator_pos);
          date_part = suffix.substr(separator_pos + 1);

          uint32_t date_part_value{0};
          if (_parse_index(index_part, parsed_file_info.index) && (date_part.size() == 8u) &&
              _parse_uint32(date_part, date_part_value))
          {
            _assign_date_suffix(parsed_file_info.date_time, date_part);
            return true;
          }
        }

        return false;
      }

      _assign_date_suffix(parsed_file_info.date_time, date_part);
      parsed_file_info.index = 0;

      if (separator_pos == NativeFilenameView::npos)
      {
        return true;
      }

      if (suffix.find('.', separator_pos + 1) != NativeFilenameView::npos)
      {
        return false;
      }

      NativeFilenameView const index_part = suffix.substr(separator_pos + 1);
      return _parse_index(index_part, parsed_file_info.index);
    }

    if (_config.rotation_naming_scheme() == RotatingFileSinkConfig::RotationNamingScheme::DateAndTime)
    {
      size_t const separator_pos = suffix.find('.');
      NativeFilenameView date_time_part =
        (separator_pos == NativeFilenameView::npos) ? suffix : suffix.substr(0, separator_pos);

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
        if (base_extension.empty() && (separator_pos != NativeFilenameView::npos) &&
            (suffix.find('.', separator_pos + 1) == NativeFilenameView::npos))
        {
          NativeFilenameView const index_part = suffix.substr(0, separator_pos);
          date_time_part = suffix.substr(separator_pos + 1);

          if (_parse_index(index_part, parsed_file_info.index) && (date_time_part.size() == 15u) &&
              (date_time_part[8] == '_') && _parse_uint32(date_time_part.substr(0, 8), date_part_value) &&
              _parse_uint32(date_time_part.substr(9), time_part_value))
          {
            _assign_date_suffix(parsed_file_info.date_time, date_time_part);
            return true;
          }
        }

        return false;
      }

      _assign_date_suffix(parsed_file_info.date_time, date_time_part);
      parsed_file_info.index = 0;

      if (separator_pos == NativeFilenameView::npos)
      {
        return true;
      }

      if (suffix.find('.', separator_pos + 1) != NativeFilenameView::npos)
      {
        return false;
      }

      NativeFilenameView const index_part = suffix.substr(separator_pos + 1);
      return _parse_index(index_part, parsed_file_info.index);
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
  static bool _rename_file(fs::path const& previous_file, fs::path const& new_file) noexcept
  {
    std::error_code ec;
    fs::rename(previous_file, new_file, ec);

#if defined(_WIN32)
    if (ec)
    {
      // Retry once after a delay - workaround for Windows antivirus locking files
      // This is a common issue where antivirus software temporarily locks files during scanning
      detail::sleep_for_ns(250ull * 1'000'000ull); // 250 ms

      ec.clear();
      fs::rename(previous_file, new_file, ec);
    }
#endif

    return !ec;
  }

  /***/
  QUILL_NODISCARD static fs::path _append_index_to_filename(fs::path const& filename, uint32_t index)
  {
    if (index == 0u)
    {
      return filename;
    }

    return _append_string_to_filename(filename, std::to_string(index));
  }

  /***/
  QUILL_NODISCARD static fs::path _append_string_to_filename(fs::path const& filename, std::string const& text)
  {
    if (text.empty())
    {
      return filename;
    }

    fs::path result = filename.parent_path() / filename.stem();
    result += ".";
    result += text;
    result += filename.extension();
    return result;
  }

  /***/
  QUILL_NODISCARD static time_t _timestamp_ns_to_time_t(uint64_t timestamp_ns)
  {
    uint64_t constexpr nanoseconds_per_second{1'000'000'000u};
    uint64_t const seconds_since_epoch{timestamp_ns / nanoseconds_per_second};

    if (QUILL_UNLIKELY(seconds_since_epoch > static_cast<uint64_t>((std::numeric_limits<time_t>::max)())))
    {
      QUILL_THROW(QuillError{"timestamp is too large to represent as time_t"});
    }

    return static_cast<time_t>(seconds_since_epoch);
  }

  /***/
  QUILL_NODISCARD static uint64_t _seconds_to_nanoseconds_saturated(uint64_t seconds) noexcept
  {
    uint64_t constexpr nanoseconds_per_second{1'000'000'000u};
    uint64_t constexpr max_timestamp_ns{(std::numeric_limits<uint64_t>::max)()};

    if (QUILL_UNLIKELY(seconds > (max_timestamp_ns / nanoseconds_per_second)))
    {
      return max_timestamp_ns;
    }

    return seconds * nanoseconds_per_second;
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
      // mktime may have normalized a nonexistent local time on the previous day.
      date.tm_hour = static_cast<decltype(date.tm_hour)>(config.daily_rotation_time().first.count());
      date.tm_min = static_cast<decltype(date.tm_min)>(config.daily_rotation_time().second.count());
      date.tm_sec = 0;
      rotation_time = to_timestamp(date);
    }

    return _seconds_to_nanoseconds_saturated(static_cast<uint64_t>(rotation_time));
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

    return _seconds_to_nanoseconds_saturated(rotation_time_seconds);
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
      uint64_t const intervals_to_next_rotation = intervals_elapsed + 1u;
      uint64_t const remaining_timestamp_range = (std::numeric_limits<uint64_t>::max)() - previous_rotation_tp;

      if (QUILL_UNLIKELY(intervals_to_next_rotation > (remaining_timestamp_range / interval_ns)))
      {
        return (std::numeric_limits<uint64_t>::max)();
      }

      return previous_rotation_tp + (intervals_to_next_rotation * interval_ns);
    }

    if (config.rotation_frequency() == RotatingFileSinkConfig::RotationFrequency::Daily)
    {
      return _calculate_next_daily_rotation_tp(_timestamp_ns_to_time_t(record_timestamp_ns), config);
    }

    QUILL_THROW(QuillError{"Invalid rotation frequency"});
  }

  /**
   * Tracks the DateAndTime backups named with `date_time` that the sink does not know about.
   * Startup in "w" mode keeps the backups of previous runs without tracking them, and a run that
   * started within the same second gives its backups the same names. Once tracked, rotation shifts
   * them to a greater index like the sink's own backups instead of renaming over them, so index 0
   * stays the newest backup, as append-mode recovery and backup retention expect.
   */
  void _track_backups_with_date_time(std::string const& date_time)
  {
    for (uint32_t index = 0;; ++index)
    {
      // Only a file is a backup: a directory with the name of one must not be renamed
      std::error_code ec;
      fs::file_status const status = fs::status(_get_filename(this->_filename, index, date_time), ec);

      if (ec || (status.type() != fs::file_type::regular))
      {
        return;
      }

      // The list follows rotation order, which is not date_time order once local time repeats at
      // the end of daylight saving time or the clock steps back, so look for the entry everywhere
      bool const already_tracked =
        std::any_of(_created_files.begin(), _created_files.end(),
                    [&date_time, index](FileInfo const& file_info)
                    { return (file_info.date_time == date_time) && (file_info.index == index); });

      if (already_tracked)
      {
        continue;
      }

      // _created_files is ordered newest first, and within one date_time a greater index is older
      auto const position =
        std::find_if(_created_files.begin(), _created_files.end(),
                     [&date_time, index](FileInfo const& file_info)
                     {
                       return !file_info.date_time.empty() &&
                         ((file_info.date_time < date_time) ||
                          ((file_info.date_time == date_time) && (file_info.index >= index)));
                     });

      _created_files.insert(position, FileInfo{this->_filename, index, date_time});
    }
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
    FileInfo(fs::path file_base_filename, uint32_t file_index, std::string file_date_time)
      : base_filename{std::move(file_base_filename)}, date_time{std::move(file_date_time)}, index{file_index}
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

  static constexpr std::chrono::seconds rotation_retry_interval{1};

  std::chrono::steady_clock::time_point _next_rotation_attempt{}; /**< Earliest retry after a failed rotation */
  std::chrono::steady_clock::time_point _next_backup_limit_check{}; /**< Earliest recheck at the backup limit */
  std::deque<FileInfo> _created_files; /**< The files this sink created or recovered on startup */
  uint64_t _next_rotation_time{0};     /**< The next rotation time point */
  uint64_t _open_file_timestamp{0};    /**< The timestamp of the currently open file */
  RotatingFileSinkConfig _config;
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
