/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/handlers/StreamHandler.h" // for StreamHandler
#include <string>                         // for string

namespace quill
{

enum class FilenameAppend : uint8_t
{
  StartDateTime,
  StartDate,
  None
};

/**
 * The FileHandlerConfig class holds the configuration options for the FileHandler
 */
class FileHandlerConfig
{
public:
  /**
   * @brief Sets the append type for the file name.
   * Possible append types are: StartDate, StartDateTime or None.
   * When this option is set, the file name will be appended with the start date or date and time
   * timestamp of when the process started.
   *
   * For example:
   * application.log -> application_20230101.log  (StartDate)
   * application.log -> application_20230101_121020.log  (StartDateTime)
   *
   * @param value The append type to set. Valid options are Date and DateAndTime.
   */
  QUILL_ATTRIBUTE_COLD void set_append_to_filename(FilenameAppend value);

  /**
   * @brief Sets the timezone to use for time-based operations e.g. when appending the date to the
   * filename or when setting the logging pattern.
   * Valid options for the timezone are 'LocalTime' or 'GmtTime'
   * The default value is 'LocalTime'
   * @param timezone The timezone to use for time-based operations.
   */
  QUILL_ATTRIBUTE_COLD void set_timezone(Timezone timezone);

  /**
   * @brief Sets whether fsync should be performed when flushing.
   * The default value is false.
   * @param value True to perform fsync, false otherwise.
   */
  QUILL_ATTRIBUTE_COLD void set_do_fsync(bool value);

  /**
   * @brief Sets the open mode for the file.
   * Valid options for the open mode are 'a' or 'w'. The default value is 'a'.
   * @param open_mode open mode for the file.
   */
  QUILL_ATTRIBUTE_COLD void set_open_mode(char open_mode);

  /**
   * @brief Sets the logging pattern for the file handler.
   * It is also possible to change the timezone with `set_timezone`
   * The default log_pattern is :
   *   "%(ascii_time) [%(thread)] %(fileline:<28) LOG_%(level_name:<9) %(logger_name:<12) %(message)"
   * @see PatternFormatter.h for more details on the pattern format.
   * @param log_pattern: Specifies the format pattern for the log messages.
   * @param time_format Specifies the format pattern for the log timestamps.
   */
  QUILL_ATTRIBUTE_COLD void set_pattern(std::string const& log_pattern,
                                        std::string const& time_format = std::string{
                                          "%H:%M:%S.%Qns"});

  /** Getters **/
  QUILL_NODISCARD bool do_fsync() const noexcept { return _do_fsync; }
  QUILL_NODISCARD Timezone timezone() const noexcept { return _timezone_value; }
  QUILL_NODISCARD FilenameAppend append_to_filename() const noexcept { return _append_to_filename; }
  QUILL_NODISCARD std::string const& open_mode() const noexcept { return _open_mode; }
  QUILL_NODISCARD std::string const& log_pattern() const noexcept { return _log_pattern; }
  QUILL_NODISCARD std::string const& time_format() const noexcept { return _time_format; }

private:
  std::string _open_mode{'a'};
  std::string _log_pattern;
  std::string _time_format;
  Timezone _timezone_value{Timezone::LocalTime};
  FilenameAppend _append_to_filename{FilenameAppend::None};
  bool _do_fsync{false};
};

/**
 * FileHandler
 * Writes the log messages to a file
 */
class FileHandler : public StreamHandler
{
public:
  /**
   * This constructor will always call fopen to open_file the given file
   * @param filename string containing the name of the file to be opened.
   * @param config Filehandler config
   * @param file_event_notifier notifies on file events
   * @param do_fopen if false the file will not be opened
   */
  FileHandler(fs::path const& filename, FileHandlerConfig config,
              FileEventNotifier file_event_notifier, bool do_fopen = true);

  ~FileHandler() override;

  /**
   * Flushes the stream and optionally fsyncs it
   */
  QUILL_ATTRIBUTE_HOT void flush() noexcept override;

protected:
  void open_file(fs::path const& filename, std::string const& mode);
  void close_file();

private:
  FileHandlerConfig _config;
};
} // namespace quill
