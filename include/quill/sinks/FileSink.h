/**
 * @page copyright
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

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#if defined(_WIN32)
  #if !defined(WIN32_LEAN_AND_MEAN)
    #define WIN32_LEAN_AND_MEAN
  #endif

  #if !defined(NOMINMAX)
    // Mingw already defines this, so no need to redefine
    #define NOMINMAX
  #endif

  #include <io.h>
  #include <share.h>
  #include <windows.h>
#else
  #include <fcntl.h>
  #include <unistd.h>
#endif

QUILL_BEGIN_NAMESPACE

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
  #pragma warning(push)
  #pragma warning(disable : 4996)
#endif

QUILL_BEGIN_EXPORT

enum class FilenameAppendOption : uint8_t
{
  None,
  StartDate,
  StartDateTime,
  StartCustomTimestampFormat
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
   * @param append_filename_format_pattern Specifies a custom `strftime` format pattern to use for the filename. This parameter is
   *                                       only applicable when `FilenameAppendOption::CustomDateTimeFormat` is selected
   */
  QUILL_ATTRIBUTE_COLD void set_filename_append_option(
    FilenameAppendOption value, std::string_view append_filename_format_pattern = std::string_view{})
  {
    _filename_append_option = value;

    if (_filename_append_option == FilenameAppendOption::StartCustomTimestampFormat)
    {
      if (append_filename_format_pattern.empty())
      {
        QUILL_THROW(QuillError{
          "The 'CustomDateTimeFormat' option was specified, but no format pattern was provided. "
          "Please set a valid strftime format pattern"});
      }

      _append_filename_format_pattern = append_filename_format_pattern;
    }
    else if (_filename_append_option == FilenameAppendOption::StartDateTime)
    {
      _append_filename_format_pattern = "_%Y%m%d_%H%M%S";
    }
    else if (_filename_append_option == FilenameAppendOption::StartDate)
    {
      _append_filename_format_pattern = "_%Y%m%d";
    }
  }

  /**
   * @brief Sets the timezone to use for time-based operations e.g. when appending the date to the
   * get_filename or when setting the logging pattern.
   * Valid options for the timezone are 'LocalTime' or 'GmtTime'
   * The default value is 'LocalTime'
   * @param time_zone The timezone to use for time-based operations.
   */
  QUILL_ATTRIBUTE_COLD void set_timezone(Timezone time_zone) { _time_zone = time_zone; }

  /**
   * @brief Sets whether fsync should be performed when flushing.
   * The default value is false.
   *
   * @note On macOS, fsync() flushes data to the disk controller's write cache but does not
   * guarantee write-through to persistent storage.
   *
   * @param value True to perform fsync, false otherwise.
   */
  QUILL_ATTRIBUTE_COLD void set_fsync_enabled(bool value) { _fsync_enabled = value; }

  /**
   * @brief Sets the open mode for the file.
   * @param open_mode open mode for the file.
   */
  QUILL_ATTRIBUTE_COLD void set_open_mode(char open_mode) { _open_mode = open_mode; }

  /**
   * @brief Sets the open mode for the file.
   * @param open_mode open mode for the file.
   */
  QUILL_ATTRIBUTE_COLD void set_open_mode(std::string_view open_mode) { _open_mode = open_mode; }

  /**
   * @brief Sets the user-defined buffer size for fwrite operations.
   *
   * This function allows you to specify a custom buffer size for fwrite, improving efficiency
   * for file write operations.
   *
   * To disable custom buffering and revert to the default size, pass a value of 0.
   *
   * @note By default, a buffer size of 64 KB is used.
   * @param value Size of the buffer in bytes. If set to 0, the default buffer size will be used.
   */
  QUILL_ATTRIBUTE_COLD void set_write_buffer_size(size_t value)
  {
    _write_buffer_size = (value == 0) ? 0 : ((value < 4096) ? 4096 : value);
  }

  /**
   * Sets the minimum interval between `fsync` calls. This specifies the minimum time between
   * consecutive `fsync` operations but does not guarantee that `fsync` will be called exactly
   * at that interval.
   *
   * For example, if some messages are flushed to the log and `fsync` is skipped because it
   * was previously called, and no further messages are written to the file, `fsync` will not
   * be called even if the minimum interval has passed. This is because the previous call was
   * skipped due to the interval, and no new messages necessitate another `fsync` call.
   *
   * This feature is intended to mitigate concerns about frequent `fsync` calls potentially causing
   * disk wear.
   *
   * Note: This option is only applicable when `fsync` is enabled. By default, the value is 0,
   * which means that `fsync` will be called periodically by the backend worker thread when
   * messages are written to the file, irrespective of the interval.
   *
   * @param value The minimum interval, in milliseconds, between `fsync` calls.
   */
  QUILL_ATTRIBUTE_COLD void set_minimum_fsync_interval(std::chrono::milliseconds value)
  {
    _minimum_fsync_interval = value;
  }

  /**
   * @brief Sets custom pattern formatter options for this sink.
   *
   * By default, the logger's pattern formatter is used to format log messages.
   * This function allows overriding the default formatter with custom options for this specific
   * sink. If a custom formatter is provided, it will be used instead of the logger's formatter.
   *
   * @param options The custom pattern formatter options to use
   */
  QUILL_ATTRIBUTE_COLD void set_override_pattern_formatter_options(std::optional<PatternFormatterOptions> const& options)
  {
    _override_pattern_formatter_options = options;
  }

  /** Getters **/
  QUILL_NODISCARD bool fsync_enabled() const noexcept { return _fsync_enabled; }
  QUILL_NODISCARD Timezone timezone() const noexcept { return _time_zone; }
  QUILL_NODISCARD FilenameAppendOption filename_append_option() const noexcept
  {
    return _filename_append_option;
  }
  QUILL_NODISCARD std::string const& append_filename_format_pattern() const noexcept
  {
    return _append_filename_format_pattern;
  }
  QUILL_NODISCARD std::string const& open_mode() const noexcept { return _open_mode; }
  QUILL_NODISCARD size_t write_buffer_size() const noexcept { return _write_buffer_size; }
  QUILL_NODISCARD std::chrono::milliseconds minimum_fsync_interval() const noexcept
  {
    return _minimum_fsync_interval;
  }
  QUILL_NODISCARD std::optional<PatternFormatterOptions> const& override_pattern_formatter_options() const noexcept
  {
    return _override_pattern_formatter_options;
  }

private:
  std::string _open_mode{'a'};
  std::string _append_filename_format_pattern;
  size_t _write_buffer_size{64 * 1024}; // Default size 64k
  std::chrono::milliseconds _minimum_fsync_interval{0};
  std::optional<PatternFormatterOptions> _override_pattern_formatter_options;
  Timezone _time_zone{Timezone::LocalTime};
  FilenameAppendOption _filename_append_option{FilenameAppendOption::None};
  bool _fsync_enabled{false};
};

/**
 * Keep the Windows-specific FileSink split isolated under _WIN32.
 * A previous attempt to share a FileSinkBase across all platforms introduced a measurable Linux
 * throughput regression in BENCHMARK_quill_backend_throughput despite equivalent logic.
 * If this is revisited, revalidate Linux performance.
 */

/**
 * FileSink
 * Writes the log messages to a file
 */
#if defined(_WIN32)
class FileSinkBase : public StreamSink
{
public:
  explicit FileSinkBase(fs::path const& filename, FileSinkConfig const& config, FileEventNotifier file_event_notifier,
                        bool /* do_fopen */, std::chrono::system_clock::time_point start_time)
    : StreamSink(_get_updated_filename_with_appended_datetime(filename, config.filename_append_option(),
                                                              config.append_filename_format_pattern(),
                                                              config.timezone(), start_time),
                 nullptr, config.override_pattern_formatter_options(), std::move(file_event_notifier)),
      _config(config)
  {
    if (!_config.fsync_enabled() && (_config.minimum_fsync_interval().count() != 0))
    {
      QUILL_THROW(
        QuillError{"Cannot set a non-zero minimum fsync interval when fsync is disabled."});
    }
  }

  ~FileSinkBase() override = default;

protected:
  QUILL_NODISCARD static std::string format_datetime_string(uint64_t timestamp_ns, Timezone time_zone,
                                                            std::string const& append_format_pattern)
  {
    auto const time_now = static_cast<time_t>(timestamp_ns / 1000000000);
    tm now_tm;

    if (time_zone == Timezone::GmtTime)
    {
      detail::gmtime_rs(&time_now, &now_tm);
    }
    else
    {
      detail::localtime_rs(&time_now, &now_tm);
    }

    static constexpr size_t buffer_size{128};
    char buffer[buffer_size];
    size_t const len = std::strftime(buffer, buffer_size, append_format_pattern.data(), &now_tm);

    return std::string{buffer, len};
  }

  QUILL_NODISCARD static std::pair<std::string, std::string> extract_stem_and_extension(fs::path const& filename) noexcept
  {
    return std::make_pair((filename.parent_path() / filename.stem()).string(), filename.extension().string());
  }

  QUILL_NODISCARD static fs::path append_datetime_to_filename(fs::path const& filename,
                                                              std::string const& append_filename_format_pattern,
                                                              Timezone time_zone,
                                                              std::chrono::system_clock::time_point timestamp) noexcept
  {
    auto const [stem, ext] = extract_stem_and_extension(filename);

    uint64_t const timestamp_ns = static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(timestamp.time_since_epoch()).count());

    return stem + format_datetime_string(timestamp_ns, time_zone, append_filename_format_pattern) + ext;
  }

private:
  QUILL_NODISCARD static fs::path _get_updated_filename_with_appended_datetime(
    fs::path const& filename, FilenameAppendOption append_to_filename_option,
    std::string const& append_filename_format_pattern, Timezone time_zone,
    std::chrono::system_clock::time_point timestamp)
  {
    if ((append_to_filename_option == FilenameAppendOption::None) || (filename == "/dev/null"))
    {
      return filename;
    }

    if ((append_to_filename_option == FilenameAppendOption::StartCustomTimestampFormat) ||
        (append_to_filename_option == FilenameAppendOption::StartDate) ||
        (append_to_filename_option == FilenameAppendOption::StartDateTime))
    {
      return append_datetime_to_filename(filename, append_filename_format_pattern, time_zone, timestamp);
    }

    QUILL_THROW(QuillError{"Unexpected FilenameAppendOption value"});
  }

protected:
  FileSinkConfig _config;
  std::chrono::steady_clock::time_point _last_fsync_timestamp{};
};

/**
 * Windows FileSink implementation.
 * Uses a native HANDLE + WriteFile path because it provides materially better
 * backend file-write throughput on Windows than the CRT FILE* path.
 */
class FileSink : public FileSinkBase
{
public:
  explicit FileSink(fs::path const& filename, FileSinkConfig const& config = FileSinkConfig{},
                    FileEventNotifier file_event_notifier = FileEventNotifier{}, bool do_fopen = true,
                    std::chrono::system_clock::time_point start_time = std::chrono::system_clock::now())
    : FileSinkBase(filename, config, std::move(file_event_notifier), do_fopen, start_time)
  {
    if (do_fopen)
    {
      open_file(_filename, _config.open_mode());
    }
  }

  ~FileSink() override { close_file(); }

  QUILL_ATTRIBUTE_HOT void flush_sink() override
  {
    if (!_write_occurred)
    {
      return;
    }

    _flush_native_write_buffer();
    _write_occurred = false;

    if (_config.fsync_enabled())
    {
      fsync_file();
    }

    std::error_code ec;
    if (!fs::exists(_filename, ec))
    {
      close_file();
      open_file(_filename, _config.open_mode());
    }
  }

  QUILL_ATTRIBUTE_HOT void write_log(MacroMetadata const* /* log_metadata */,
                                     uint64_t /* log_timestamp */, std::string_view /* thread_id */,
                                     std::string_view /* thread_name */, std::string const& /* process_id */,
                                     std::string_view /* logger_name */, LogLevel /* log_level */,
                                     std::string_view /* log_level_description */,
                                     std::string_view /* log_level_short_code */,
                                     std::vector<std::pair<std::string, std::string>> const* /* named_args */,
                                     std::string_view /* log_message */, std::string_view log_statement) override
  {
    if (QUILL_UNLIKELY(_native_file_handle == INVALID_HANDLE_VALUE))
    {
      return;
    }

    std::string_view statement = log_statement;
    std::string user_log_statement;

    if (_file_event_notifier.before_write)
    {
      user_log_statement = _file_event_notifier.before_write(log_statement);
      statement = user_log_statement;
    }

    _native_write_buffer.insert(_native_write_buffer.end(), statement.begin(), statement.end());
    _file_size += statement.size();
    _write_occurred = true;

    if (_native_write_buffer.size() >= _native_write_buffer_capacity())
    {
      _flush_native_write_buffer();
    }
  }

protected:
  void open_file(fs::path const& filename, std::string const& mode)
  {
    if (_file_event_notifier.before_open)
    {
      _file_event_notifier.before_open(filename);
    }

    constexpr int max_retries = 3;
    constexpr int retry_delay_ms = 200;
    HANDLE native_file_handle = INVALID_HANDLE_VALUE;

    for (int attempt = 0; attempt < max_retries; ++attempt)
    {
      DWORD creation_disposition = OPEN_ALWAYS;
      if (!mode.empty() && mode[0] == 'w')
      {
        creation_disposition = CREATE_ALWAYS;
      }

      native_file_handle = ::CreateFileW(filename.c_str(), GENERIC_WRITE,
                                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                         nullptr, creation_disposition, FILE_ATTRIBUTE_NORMAL, nullptr);

      if (native_file_handle != INVALID_HANDLE_VALUE)
      {
        if (!::SetHandleInformation(native_file_handle, HANDLE_FLAG_INHERIT, 0))
        {
          ::CloseHandle(native_file_handle);
          native_file_handle = INVALID_HANDLE_VALUE;
        }
        else if (!mode.empty() && mode[0] == 'a')
        {
          LARGE_INTEGER zero{};
          if (!::SetFilePointerEx(native_file_handle, zero, nullptr, FILE_END))
          {
            ::CloseHandle(native_file_handle);
            native_file_handle = INVALID_HANDLE_VALUE;
          }
        }
      }

      if (native_file_handle != INVALID_HANDLE_VALUE)
      {
        break;
      }

      if (attempt < max_retries - 1)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds{retry_delay_ms});
      }
    }

    if (native_file_handle == INVALID_HANDLE_VALUE)
    {
      QUILL_THROW(QuillError{std::string{"CreateFileW failed after "} + std::to_string(max_retries) +
                             " attempts, path: " + filename.string() + " mode: " + mode +
                             " GetLastError: " + std::to_string(::GetLastError())});
    }

    _native_write_buffer.clear();
    _native_write_buffer.reserve(_native_write_buffer_capacity());

    if (_file_event_notifier.after_open)
    {
      QUILL_TRY { _file_event_notifier.after_open(filename, native_file_handle); }
  #if !defined(QUILL_NO_EXCEPTIONS)
      QUILL_CATCH(...)
      {
        ::CloseHandle(native_file_handle);
        throw;
      }
  #endif
    }

    _native_file_handle = native_file_handle;
  }

  void close_file()
  {
    if (_native_file_handle == INVALID_HANDLE_VALUE)
    {
      return;
    }

    if (_file_event_notifier.before_close)
    {
      _file_event_notifier.before_close(_filename, _native_file_handle);
    }

    _flush_native_write_buffer();
    ::CloseHandle(_native_file_handle);
    _native_file_handle = INVALID_HANDLE_VALUE;
    _native_write_buffer.clear();

    if (_file_event_notifier.after_close)
    {
      _file_event_notifier.after_close(_filename);
    }
  }

  void fsync_file(bool force_fsync = false) noexcept
  {
    if (!force_fsync)
    {
      auto const now = std::chrono::steady_clock::now();
      if ((now - _last_fsync_timestamp) < _config.minimum_fsync_interval())
      {
        return;
      }
      _last_fsync_timestamp = now;
    }

    FlushFileBuffers(_native_file_handle);
  }

  void _flush_native_write_buffer()
  {
    if ((_native_file_handle == INVALID_HANDLE_VALUE) || _native_write_buffer.empty())
    {
      return;
    }

    char const* data = _native_write_buffer.data();
    size_t remaining = _native_write_buffer.size();

    while (remaining != 0)
    {
      DWORD bytes_written = 0;
      DWORD const chunk =
        static_cast<DWORD>(std::min<size_t>(remaining, std::numeric_limits<DWORD>::max()));
      if (!::WriteFile(_native_file_handle, data, chunk, &bytes_written, nullptr) || (bytes_written == 0))
      {
        QUILL_THROW(QuillError{std::string{"WriteFile failed. GetLastError: "} +
                               std::to_string(::GetLastError())});
      }

      data += bytes_written;
      remaining -= bytes_written;
    }

    _native_write_buffer.clear();
  }

  QUILL_NODISCARD size_t _native_write_buffer_capacity() const noexcept
  {
    return _config.write_buffer_size() == 0 ? default_write_buffer_size : _config.write_buffer_size();
  }

protected:
  static constexpr size_t default_write_buffer_size{64 * 1024};
  HANDLE _native_file_handle{INVALID_HANDLE_VALUE};
  std::vector<char> _native_write_buffer;
};
#else
class FileSink : public StreamSink
{
public:
  explicit FileSink(fs::path const& filename, FileSinkConfig const& config = FileSinkConfig{},
                    FileEventNotifier file_event_notifier = FileEventNotifier{}, bool do_fopen = true,
                    std::chrono::system_clock::time_point start_time = std::chrono::system_clock::now())
    : StreamSink(_get_updated_filename_with_appended_datetime(filename, config.filename_append_option(),
                                                              config.append_filename_format_pattern(),
                                                              config.timezone(), start_time),
                 nullptr, config.override_pattern_formatter_options(), std::move(file_event_notifier)),
      _config(config)
  {
    if (!_config.fsync_enabled() && (_config.minimum_fsync_interval().count() != 0))
    {
      QUILL_THROW(
        QuillError{"Cannot set a non-zero minimum fsync interval when fsync is disabled."});
    }

    if (do_fopen)
    {
      open_file(_filename, _config.open_mode());
    }
  }

  ~FileSink() override { close_file(); }

  QUILL_ATTRIBUTE_HOT void flush_sink() override
  {
    if (!_write_occurred || !_file)
    {
      return;
    }

    StreamSink::flush_sink();

    if (_config.fsync_enabled())
    {
      fsync_file();
    }

    std::error_code ec;
    if (!fs::exists(_filename, ec))
    {
      close_file();
      open_file(_filename, _config.open_mode());
    }
  }

private:
  struct OpenedFileGuard
  {
    ~OpenedFileGuard()
    {
      if (file)
      {
        std::fclose(file);
      }
    }

    QUILL_NODISCARD FILE* release() noexcept
    {
      FILE* const released_file = file;
      file = nullptr;
      return released_file;
    }

    FILE* file{nullptr};
  };

protected:
  QUILL_NODISCARD static std::string format_datetime_string(uint64_t timestamp_ns, Timezone time_zone,
                                                            std::string const& append_format_pattern)
  {
    auto const time_now = static_cast<time_t>(timestamp_ns / 1000000000);
    tm now_tm;

    if (time_zone == Timezone::GmtTime)
    {
      detail::gmtime_rs(&time_now, &now_tm);
    }
    else
    {
      detail::localtime_rs(&time_now, &now_tm);
    }

    static constexpr size_t buffer_size{128};
    char buffer[buffer_size];
    size_t const len = std::strftime(buffer, buffer_size, append_format_pattern.data(), &now_tm);

    return std::string{buffer, len};
  }

  QUILL_NODISCARD static std::pair<std::string, std::string> extract_stem_and_extension(fs::path const& filename) noexcept
  {
    return std::make_pair((filename.parent_path() / filename.stem()).string(), filename.extension().string());
  }

  QUILL_NODISCARD static fs::path append_datetime_to_filename(fs::path const& filename,
                                                              std::string const& append_filename_format_pattern,
                                                              Timezone time_zone,
                                                              std::chrono::system_clock::time_point timestamp) noexcept
  {
    auto const [stem, ext] = extract_stem_and_extension(filename);

    uint64_t const timestamp_ns = static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(timestamp.time_since_epoch()).count());

    return stem + format_datetime_string(timestamp_ns, time_zone, append_filename_format_pattern) + ext;
  }

  void open_file(fs::path const& filename, std::string const& mode)
  {
    if (_file_event_notifier.before_open)
    {
      _file_event_notifier.before_open(filename);
    }

    constexpr int max_retries = 3;
    constexpr int retry_delay_ms = 200;
    OpenedFileGuard opened_file_guard;

    for (int attempt = 0; attempt < max_retries; ++attempt)
    {
      int flags = O_CREAT | O_WRONLY | O_CLOEXEC;
      flags |= (!mode.empty() && mode[0] == 'w') ? O_TRUNC : O_APPEND;

      int fd = ::open(filename.string().data(), flags, 0644);
      if (fd != -1)
      {
        opened_file_guard.file = ::fdopen(fd, mode.data());
        if (!opened_file_guard.file)
        {
          ::close(fd);
        }
      }

      if (opened_file_guard.file)
      {
        break;
      }

      if (attempt < max_retries - 1)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds{retry_delay_ms});
      }
    }

    if (!opened_file_guard.file)
    {
      QUILL_THROW(QuillError{std::string{"fopen failed after "} + std::to_string(max_retries) +
                             " attempts, path: " + filename.string() + " mode: " + mode +
                             " errno: " + std::to_string(errno) + " error: " + std::strerror(errno)});
    }

    std::unique_ptr<char[]> write_buffer;
    if (_config.write_buffer_size() != 0)
    {
      write_buffer = std::make_unique<char[]>(_config.write_buffer_size());

      if (setvbuf(opened_file_guard.file, write_buffer.get(), _IOFBF, _config.write_buffer_size()) != 0)
      {
        QUILL_THROW(QuillError{std::string{"setvbuf failed error: "} + std::strerror(errno)});
      }
    }

    if (_file_event_notifier.after_open)
    {
      _file_event_notifier.after_open(filename, opened_file_guard.file);
    }

    _file = opened_file_guard.release();
    _write_buffer = std::move(write_buffer);
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

  void fsync_file(bool force_fsync = false) noexcept
  {
    if (!_file)
    {
      return;
    }

    if (!force_fsync)
    {
      auto const now = std::chrono::steady_clock::now();
      if ((now - _last_fsync_timestamp) < _config.minimum_fsync_interval())
      {
        return;
      }
      _last_fsync_timestamp = now;
    }

    ::fsync(fileno(_file));
  }

private:
  QUILL_NODISCARD static fs::path _get_updated_filename_with_appended_datetime(
    fs::path const& filename, FilenameAppendOption append_to_filename_option,
    std::string const& append_filename_format_pattern, Timezone time_zone,
    std::chrono::system_clock::time_point timestamp)
  {
    if ((append_to_filename_option == FilenameAppendOption::None) || (filename == "/dev/null"))
    {
      return filename;
    }

    if ((append_to_filename_option == FilenameAppendOption::StartCustomTimestampFormat) ||
        (append_to_filename_option == FilenameAppendOption::StartDate) ||
        (append_to_filename_option == FilenameAppendOption::StartDateTime))
    {
      return append_datetime_to_filename(filename, append_filename_format_pattern, time_zone, timestamp);
    }

    QUILL_THROW(QuillError{"Unexpected FilenameAppendOption value"});
  }

protected:
  FileSinkConfig _config;
  std::chrono::steady_clock::time_point _last_fsync_timestamp{};
  std::unique_ptr<char[]> _write_buffer;
};
#endif

QUILL_END_EXPORT

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
  #pragma warning(pop)
#endif

QUILL_END_NAMESPACE
