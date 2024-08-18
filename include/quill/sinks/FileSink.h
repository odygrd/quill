/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Common.h"
#include "quill/core/Filesystem.h"
#include "quill/core/QuillError.h"
#include "quill/core/TimeUtilities.h"
#include "quill/sinks/StreamSink.h"

#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <utility>

#if defined(_WIN32)
  #if !defined(WIN32_LEAN_AND_MEAN)
    #define WIN32_LEAN_AND_MEAN
  #endif

  #if !defined(NOMINMAX)
    // Mingw already defines this, so no need to redefine
    #define NOMINMAX
  #endif

  #include <io.h>
  #include <windows.h>
#elif defined(__APPLE__)
  #include <unistd.h>
#elif defined(__CYGWIN__)
  #include <unistd.h>
#elif defined(__linux__)
  #include <unistd.h>
#elif defined(__NetBSD__)
  #include <unistd.h>
#elif defined(__FreeBSD__)
  #include <unistd.h>
#elif defined(__DragonFly__)
  #include <unistd.h>
#else
  #include <unistd.h>
#endif

QUILL_BEGIN_NAMESPACE

enum class FilenameAppendOption : uint8_t
{
  StartDateTime,
  StartDate,
  None
};

/**
 * The FileSinkConfig class holds the configuration options for the FileSink
 */
class FileSinkConfig
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
  QUILL_ATTRIBUTE_COLD void set_filename_append_option(FilenameAppendOption value)
  {
    _filename_append_option = value;
  }

  /**
   * @brief Sets the timezone to use for time-based operations e.g. when appending the date to the
   * get_filename or when setting the logging pattern.
   * Valid options for the timezone are 'LocalTime' or 'GmtTime'
   * The default value is 'LocalTime'
   * @param timezone The timezone to use for time-based operations.
   */
  QUILL_ATTRIBUTE_COLD void set_timezone(Timezone timezone) { _time_zone = timezone; }

  /**
   * @brief Sets whether fsync should be performed when flushing.
   * The default value is false.
   * @param value True to perform fsync, false otherwise.
   */
  QUILL_ATTRIBUTE_COLD void set_do_fsync(bool value) { _fsync_enabled = value; }

  /**
   * @brief Sets the open mode for the file.
   * Valid options for the open mode are 'a' or 'w'. The default value is 'a'.
   * @param open_mode open mode for the file.
   */
  QUILL_ATTRIBUTE_COLD void set_open_mode(char open_mode) { _open_mode = open_mode; }

  /** Getters **/
  QUILL_NODISCARD bool fsync_enabled() const noexcept { return _fsync_enabled; }
  QUILL_NODISCARD Timezone timezone() const noexcept { return _time_zone; }
  QUILL_NODISCARD FilenameAppendOption filename_append_option() const noexcept
  {
    return _filename_append_option;
  }
  QUILL_NODISCARD std::string const& open_mode() const noexcept { return _open_mode; }

private:
  std::string _open_mode{'w'};
  Timezone _time_zone{Timezone::LocalTime};
  FilenameAppendOption _filename_append_option{FilenameAppendOption::None};
  bool _fsync_enabled{false};
};

/**
 * FileSink
 * Writes the log messages to a file
 */
class FileSink : public StreamSink
{
public:
  /**
   * Construct a FileSink object.
   * This constructor will always attempt to open the given file.
   * @param filename Path to the file to be opened.
   * @param config Configuration for the FileSink.
   * @param file_event_notifier Notifies on file events.
   * @param do_fopen If false, the file will not be opened.
   */
  explicit FileSink(fs::path const& filename, FileSinkConfig const& config = FileSinkConfig{},
                    FileEventNotifier file_event_notifier = FileEventNotifier{}, bool do_fopen = true,
                    std::chrono::system_clock::time_point start_time = std::chrono::system_clock::now())
    : StreamSink(_get_updated_filename_with_appended_datetime(
                   filename, config.filename_append_option(), config.timezone(), start_time),
                 nullptr, std::move(file_event_notifier)),
      _config(config)
  {
    if (do_fopen)
    {
      open_file(_filename, _config.open_mode());
    }
  }

  ~FileSink() override { close_file(); }

  /**
   * Flushes the stream and optionally fsyncs it.
   */
  QUILL_ATTRIBUTE_HOT void flush_sink() override
  {
    if (!_write_occurred || !_file)
    {
      // Check here because StreamSink::flush() will set _write_occurred to false
      return;
    }

    StreamSink::flush_sink();

    if (_config.fsync_enabled())
    {
      fsync_file(_file);
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
  /**
   * Format a datetime string.
   * @param timestamp_ns Timestamp in nanoseconds.
   * @param timezone Timezone to use.
   * @param with_time Include time in the string if true.
   * @return Formatted datetime string.
   */
  QUILL_NODISCARD static std::string format_datetime_string(uint64_t timestamp_ns, Timezone timezone, bool with_time)
  {
    // convert to seconds
    auto const time_now = static_cast<time_t>(timestamp_ns / 1000000000);
    tm now_tm;

    if (timezone == Timezone::GmtTime)
    {
      detail::gmtime_rs(&time_now, &now_tm);
    }
    else
    {
      detail::localtime_rs(&time_now, &now_tm);
    }

    // Construct the string
    char buffer[32];
    if (with_time)
    {
      std::snprintf(buffer, sizeof(buffer), "%04d%02d%02d_%02d%02d%02d", now_tm.tm_year + 1900,
                    now_tm.tm_mon + 1, now_tm.tm_mday, now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec);
    }
    else
    {
      std::snprintf(buffer, sizeof(buffer), "%04d%02d%02d", now_tm.tm_year + 1900,
                    now_tm.tm_mon + 1, now_tm.tm_mday);
    }

    return std::string{buffer};
  }

  /**
   * Extract stem and extension from a filename.
   * @param filename Path to the file.
   * @return Pair containing stem and extension.
   */
  QUILL_NODISCARD static std::pair<std::string, std::string> extract_stem_and_extension(fs::path const& filename) noexcept
  {
    // filename and extension
    return std::make_pair((filename.parent_path() / filename.stem()).string(), filename.extension().string());
  }

  /**
   * Append date and/or time to a filename.
   * @param filename Path to the file.
   * @param with_time Include time in the filename if true.
   * @param timezone Timezone to use.
   * @param timestamp Timestamp to use.
   * @return Updated filename.
   */
  QUILL_NODISCARD static fs::path append_datetime_to_filename(fs::path const& filename,
                                                              bool with_time, Timezone timezone,
                                                              std::chrono::system_clock::time_point timestamp) noexcept
  {
    // Get base file and extension
    auto const [stem, ext] = extract_stem_and_extension(filename);

    // Get the time now as tm from user or default to now
    uint64_t const timestamp_ns = static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(timestamp.time_since_epoch()).count());

    // Construct a filename
    return stem + "_" + format_datetime_string(timestamp_ns, timezone, with_time) + ext;
  }

  /**
   * Open a file.
   * @param filename Path to the file.
   * @param mode File open mode.
   */
  void open_file(fs::path const& filename, std::string const& mode)
  {
    if (_file_event_notifier.before_open)
    {
      _file_event_notifier.before_open(filename);
    }

    _file = fopen(filename.string().data(), mode.data());

    if (!_file)
    {
      QUILL_THROW(QuillError{std::string{"fopen failed failed path: "} + filename.string() + " mode: " + mode +
                             " errno: " + std::to_string(errno) + " error: " + strerror(errno)});
    }

    assert(_file && "open_file always returns a valid pointer or throws");

    if (_file_event_notifier.after_open)
    {
      _file_event_notifier.after_open(filename, _file);
    }
  }

  /**
   * Close the file.
   */
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

  /**
   * Fsync the file descriptor.
   * @param fd File descriptor.
   * @return True if successful, false otherwise.
   */
  static bool fsync_file(FILE* fd) noexcept
  {
#ifdef _WIN32
    return FlushFileBuffers(reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(fd)))) != 0;
#else
    return ::fsync(fileno(fd)) == 0;
#endif
  }

private:
  /**
   * Get the filename with appended date and/or time.
   * @param filename Path to the file.
   * @param append_to_filename_option Append option.
   * @param timezone Timezone to use.
   * @return Updated filename.
   */
  QUILL_NODISCARD static quill::fs::path _get_updated_filename_with_appended_datetime(
    quill::fs::path const& filename, quill::FilenameAppendOption append_to_filename_option,
    quill::Timezone timezone, std::chrono::system_clock::time_point timestamp)
  {
    if ((append_to_filename_option == quill::FilenameAppendOption::None) || (filename == "/dev/null"))
    {
      return filename;
    }

    if (append_to_filename_option == quill::FilenameAppendOption::StartDate)
    {
      return append_datetime_to_filename(filename, false, timezone, timestamp);
    }

    if (append_to_filename_option == quill::FilenameAppendOption::StartDateTime)
    {
      return append_datetime_to_filename(filename, true, timezone, timestamp);
    }

    return quill::fs::path{};
  }

private:
  FileSinkConfig _config;
};

QUILL_END_NAMESPACE