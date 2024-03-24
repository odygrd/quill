/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/handlers/StreamHandler.h" // for StreamHandler
#include <string>                         // for string

#include "quill/common/FileUtilities.h" // for append_date_to_filename
#include "quill/common/Fmt.h"
#include "quill/common/Os.h"
#include <cstdio> // for fclose

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
  QUILL_ATTRIBUTE_COLD void set_append_to_filename(FilenameAppend value)
  {
    _append_to_filename = value;
  }

  /**
   * @brief Sets the timezone to use for time-based operations e.g. when appending the date to the
   * filename or when setting the logging pattern.
   * Valid options for the timezone are 'LocalTime' or 'GmtTime'
   * The default value is 'LocalTime'
   * @param timezone The timezone to use for time-based operations.
   */
  QUILL_ATTRIBUTE_COLD void set_timezone(Timezone timezone) { _timezone_value = timezone; }

  /**
   * @brief Sets whether fsync should be performed when flushing.
   * The default value is false.
   * @param value True to perform fsync, false otherwise.
   */
  QUILL_ATTRIBUTE_COLD void set_do_fsync(bool value) { _do_fsync = value; }

  /**
   * @brief Sets the open mode for the file.
   * Valid options for the open mode are 'a' or 'w'. The default value is 'a'.
   * @param open_mode open mode for the file.
   */
  QUILL_ATTRIBUTE_COLD void set_open_mode(char open_mode) { _open_mode = open_mode; }

  /**
   * @brief Sets the logging pattern for the file handler.
   * It is also possible to change the timezone with `set_timezone`
   * The default log_pattern is :
   *   "%(time) [%(thread_id)] %(short_source_location:<28) LOG_%(log_level:<9) %(logger:<12) %(message)"
   * @see PatternFormatter.h for more details on the pattern format.
   * @param log_pattern: Specifies the format pattern for the log messages.
   * @param time_format Specifies the format pattern for the log timestamps.
   */
  QUILL_ATTRIBUTE_COLD void set_pattern(std::string const& log_pattern,
                                        std::string const& time_format = std::string{
                                          "%H:%M:%S.%Qns"})
  {
    _log_pattern = log_pattern;
    _time_format = time_format;
  }

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
  FileHandler(fs::path const& filename, FileHandlerConfig const& config,
              FileEventNotifier file_event_notifier, bool do_fopen = true)
    : StreamHandler(_get_appended_filename(filename, config.append_to_filename(), config.timezone()),
                    nullptr, std::move(file_event_notifier)),
      _config(config)
  {
    if (!_config.log_pattern().empty())
    {
      set_pattern(_config.log_pattern(), _config.time_format());
    }

    if (do_fopen)
    {
      open_file(_filename, _config.open_mode());
    }
  }

  ~FileHandler() override { close_file(); }

  /**
   * Flushes the stream and optionally fsyncs it
   */
  QUILL_ATTRIBUTE_HOT void flush() override
  {
    if (!_write_occurred || !_file)
    {
      // Check here because StreamHandler::flush() will set _write_occurred to false
      return;
    }

    StreamHandler::flush();

    if (_config.do_fsync())
    {
      detail::fsync(_file);
    }

    if (!fs::exists(_filename))
    {
      // after flushing the file we can check if the file still exists. If not we reopen it.
      // This can happen if a user deletes a file while the application is running
      close_file();

      // now reopen the file for writing again, it will be a new file
      open_file(_filename, "w");
    }
  }

protected:
  void open_file(fs::path const& filename, std::string const& mode)
  {
    if (_file_event_notifier.before_open)
    {
      _file_event_notifier.before_open(filename);
    }

    // _file is the base file*
    _file = detail::open_file(filename, mode);

    assert(_file && "open_file always returns a valid pointer or throws");

    if (_file_event_notifier.after_open)
    {
      _file_event_notifier.after_open(filename, _file);
    }
  }

  void close_file()
  {
    if (!_file)
    {
      return;
    }

    if (_file_event_notifier.before_close)
    {
      _file_event_notifier.before_close(_filename, _file);
    }

    fclose(_file);
    _file = nullptr;

    if (_file_event_notifier.after_close)
    {
      _file_event_notifier.after_close(_filename);
    }
  }

private:
  QUILL_NODISCARD quill::fs::path _get_appended_filename(quill::fs::path const& filename,
                                                         quill::FilenameAppend append_to_filename,
                                                         quill::Timezone timezone)
  {
    if ((append_to_filename == quill::FilenameAppend::None) || (filename == "/dev/null"))
    {
      return filename;
    }

    if (append_to_filename == quill::FilenameAppend::StartDate)
    {
      return quill::detail::append_date_time_to_filename(filename, false, timezone);
    }

    if (append_to_filename == quill::FilenameAppend::StartDateTime)
    {
      return quill::detail::append_date_time_to_filename(filename, true, timezone);
    }

    return quill::fs::path{};
  }

private:
  FileHandlerConfig _config;
};
} // namespace quill
