/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Attributes.h" // for QUILL_ATTRIBUTE_COLD, QUIL...
#include "quill/handlers/FileHandler.h"   // for FileHandler
#include <chrono>                         // for nanoseconds
#include <cstddef>                        // for size_t
#include <cstdint>                        // for uint32_t
#include <deque>
#include <limits>
#include <sstream>
#include <utility>
#include <vector>

namespace quill
{

/**
 * The RotatingFileHandlerConfig class holds the configuration options for the RotatingFileHandler
 */
class RotatingFileHandlerConfig : public FileHandlerConfig
{
public:
  enum class RotationFrequency : uint8_t
  {
    Disabled,
    Daily,
    Hourly,
    Minutely
  };

  enum class RotationNamingScheme : uint8_t
  {
    Index,
    Date,
    DateAndTime
  };

  RotatingFileHandlerConfig();

  /**
   * @brief Sets the maximum file size in bytes. Enabling this option will enable file rotation by file size. By default this is disabled.
   * @param value The maximum file size in bytes per file
   */
  QUILL_ATTRIBUTE_COLD void set_rotation_max_file_size(size_t value);

  /**
   * @brief Sets the frequency and interval of file rotation.
   * Enabling this option will enable file rotation based on a specified frequency and interval.
   * By default, this option is disabled.
   * Valid values for the frequency are 'M' for minutes and 'H' for hours.
   * @param frequency The frequency of file rotation to set.
   * @param interval The rotation interval to set.
   */
  QUILL_ATTRIBUTE_COLD void set_rotation_frequency_and_interval(char frequency, uint32_t interval);

  /**
   * @brief Sets the time of day for daily log file rotation.
   * When this option is set, the rotation frequency is automatically set to 'daily'.
   * By default, this option is disabled.
   * @param time The time of day to perform the log file rotation. The value must be in the format "HH:MM".
   */
  QUILL_ATTRIBUTE_COLD void set_rotation_time_daily(std::string const& at_time);

  /**
   * @brief Sets the maximum number of log files to keep. By default, there is no limit on the number of log files.
   * @param value The maximum number of log files to set.
   */
  QUILL_ATTRIBUTE_COLD void set_max_backup_files(uint32_t value);

  /**
   * @brief Sets whether the oldest rolled logs should be overwritten when the maximum backup count
   * is reached. If set to false, the oldest logs will not be overwritten when the maximum backup
   * count is reached, and log file rotation will stop. The default value is true.
   * @param value True to overwrite the oldest logs, false otherwise.
   */
  QUILL_ATTRIBUTE_COLD void set_overwrite_rolled_files(bool value);

  /**
   * @brief Sets whether previous rotated log files should be removed on process start up.
   * @note This option works only when using the mode="w"
   * This is useful to avoid conflicting file names when the process restarts and
   * FilenameAppend::DateTime was not set. The default value is true.
   * @param value True to remove old log files, false otherwise.
   */
  QUILL_ATTRIBUTE_COLD void set_remove_old_files(bool value);

  /**
   * @brief Sets the naming scheme for the rotated files.
   * The default value is 'Index'.
   * @param value The naming scheme to set.
   */
  QUILL_ATTRIBUTE_COLD void set_rotation_naming_scheme(RotationNamingScheme value);

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
  QUILL_NODISCARD std::pair<std::chrono::hours, std::chrono::minutes> rotation_at_time_daily() const noexcept
  {
    return _rotation_at_time_daily;
  }
  QUILL_NODISCARD RotationNamingScheme rotation_naming_scheme() const noexcept
  {
    return _rotation_naming_scheme;
  }

private:
  std::pair<std::chrono::hours, std::chrono::minutes> _rotation_at_time_daily;
  size_t _rotation_max_file_size{0};                                // 0 means disabled
  uint32_t _max_backup_files{std::numeric_limits<uint32_t>::max()}; // max means disabled
  uint32_t _rotation_interval{0};                                   // 0 means disabled
  RotationFrequency _rotation_frequency{RotationFrequency::Disabled};
  RotationNamingScheme _rotation_naming_scheme{RotationNamingScheme::Index};
  bool _overwrite_rolled_files{true};
  bool _remove_old_files{true};
};

/**
 * @brief The RotatingFileHandler class
 */
class RotatingFileHandler : public FileHandler
{
public:
  /**
   * @brief Constructor.
   *
   * Creates a new instance of the RotatingFileHandler class.
   *
   * @param filename The base file name to be used for logs.
   * @param mode The mode to open the file.
   * @param config The handler configuration.
   */
  RotatingFileHandler(fs::path const& filename, RotatingFileHandlerConfig const& config,
                      FileEventNotifier file_event_notifier,
                      std::chrono::system_clock::time_point start_time = std::chrono::system_clock::now());

  /**
   * @brief Destructor.
   *
   * Destroys the RotatingFileHandler object.
   */
  ~RotatingFileHandler() override = default;

  /**
   * @brief Write a formatted log message to the stream.
   *
   * This function writes a formatted log message to the file stream associated with the handler.
   *
   * @param formatted_log_message The formatted log message to write.
   * @param log_event The log event associated with the message.
   */
  QUILL_ATTRIBUTE_HOT void write(fmt_buffer_t const& formatted_log_message,
                                 quill::TransitEvent const& log_event) override;

private:
  QUILL_NODISCARD bool _time_rotation(uint64_t record_timestamp_ns);
  void _size_rotation(size_t log_msg_size, uint64_t record_timestamp_ns);
  void _rotate_files(uint64_t record_timestamp_ns);
  void _clean_and_recover_files(fs::path const& filename, std::string const& open_mode, uint64_t today_timestamp_ns);

private:
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

private:
  FileEventNotifier _file_event_notifier;
  std::deque<FileInfo> _created_files; /**< We store in a queue the filenames we created, first: index, second: date/datetime, third: base_filename */
  uint64_t _next_rotation_time;        /**< The next rotation time point */
  uint64_t _open_file_timestamp{0};    /**< The timestamp of the currently open file */
  size_t _file_size{0};                /**< The current file size */
  RotatingFileHandlerConfig _config;
};

} // namespace quill
