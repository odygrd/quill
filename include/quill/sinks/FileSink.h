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
#include "quill/core/ThreadPrimitives.h"
#include "quill/core/TimeUtilities.h"
#include "quill/sinks/StreamSink.h"

#ifndef QUILL_MODULE
  #include <cerrno>
  #include <chrono>
  #include <cstdint>
  #include <cstdio>
  #include <cstring>
  #include <ctime>
  #include <memory>
  #include <string>
  #include <string_view>
  #include <utility>
#endif

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
  #include <sys/stat.h>
  #include <unistd.h>
#endif

QUILL_BEGIN_NAMESPACE

namespace detail
{
class SinkManager;
}

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
   * Possible append types are: None, StartDate, StartDateTime or StartCustomTimestampFormat.
   * When this option is set, the file name is appended with the date, date and time, or custom
   * timestamp of when the sink was created.
   *
   * For example:
   * application.log -> application_20230101.log  (StartDate)
   * application.log -> application_20230101_121020.log  (StartDateTime)
   *
   * @param value The append type to set. Valid options are None, StartDate, StartDateTime, and StartCustomTimestampFormat.
   * @param append_filename_format_pattern Specifies a custom `strftime` format pattern to use for the filename. This parameter is
   *                                       only applicable when `FilenameAppendOption::StartCustomTimestampFormat` is selected
   */
  QUILL_ATTRIBUTE_COLD void set_filename_append_option(
    FilenameAppendOption value, std::string_view append_filename_format_pattern = std::string_view{})
  {
    _filename_append_option = value;

    if (_filename_append_option == FilenameAppendOption::StartCustomTimestampFormat)
    {
      if (append_filename_format_pattern.empty())
      {
        QUILL_THROW(
          QuillError{"The 'StartCustomTimestampFormat' option was specified, but no format pattern "
                     "was provided. "
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
   * @brief Sets the timezone of the timestamp appended to the file name and, for rotating sinks,
   * of the rotation schedule. The log pattern's timezone is set in PatternFormatterOptions.
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
   * @brief Sets the open mode for the file: 'a' appends to an existing file, 'w' truncates it.
   * @param open_mode 'a' or 'w'
   * @throws QuillError for any other mode
   */
  QUILL_ATTRIBUTE_COLD void set_open_mode(char open_mode)
  {
    set_open_mode(std::string_view{&open_mode, 1});
  }

  /**
   * @brief Sets the open mode for the file: "a" appends to an existing file, "w" truncates it.
   * "ab" and "wb" are accepted as aliases, since the file is always written as binary.
   * @param open_mode "a", "w", "ab" or "wb"
   * @throws QuillError for any other mode
   */
  QUILL_ATTRIBUTE_COLD void set_open_mode(std::string_view open_mode)
  {
    if ((open_mode != "a") && (open_mode != "w") && (open_mode != "ab") && (open_mode != "wb"))
    {
      QUILL_THROW(QuillError{"Invalid file open mode \"" + std::string{open_mode} +
                             "\", use \"a\" to append or \"w\" to truncate"});
    }

    _open_mode = open_mode;
  }

  /**
   * @brief Sets the user-defined buffer size for fwrite operations.
   *
   * This function allows you to specify a custom buffer size for fwrite, improving efficiency
   * for file write operations.
   *
   * To disable custom buffering and revert to the default size, pass a value of 0.
   *
   * @note By default, a buffer of 64 KiB is used, or of 256 KiB on Windows.
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
   * An `fsync` skipped because of the interval is performed by a later flush once the interval
   * has elapsed, even if no further messages are written, and by the final flush when the
   * backend stops.
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
    if (value.count() < 0)
    {
      QUILL_THROW(QuillError{"minimum_fsync_interval must not be negative"});
    }

    _minimum_fsync_interval = value;
  }

  /**
   * @brief Sets the bytes that start every file of the sink, e.g. the header row of a csv file or
   * the global header of a pcap file. The default is no header.
   *
   * The header is written when the sink opens a file that is empty, which includes each new file
   * of a rotating sink. A file that is opened for appending and already has content keeps it and
   * gets no header. The pattern formatter does not apply to the header, so a line of text needs
   * its own newline. FileEventNotifier::before_write does, as to everything the sink writes.
   *
   * FileEventNotifier::after_open is called before the header is written, with the file as it
   * was opened. Bytes that the callback writes to a new file therefore precede the header: make
   * them part of the header when they belong after its start.
   *
   * @param file_header The header, or empty for none.
   */
  QUILL_ATTRIBUTE_COLD void set_file_header(std::string_view file_header)
  {
    _file_header = file_header;
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
  QUILL_NODISCARD std::string const& file_header() const noexcept { return _file_header; }
  QUILL_NODISCARD size_t write_buffer_size() const noexcept { return _write_buffer_size; }
  QUILL_NODISCARD std::chrono::milliseconds minimum_fsync_interval() const noexcept
  {
    return _minimum_fsync_interval;
  }
  QUILL_NODISCARD std::optional<PatternFormatterOptions> const& override_pattern_formatter_options() const noexcept
  {
    return _override_pattern_formatter_options;
  }

public:
  /**
   * The write buffer size that is used by default. A write to a file takes a few microseconds on
   * Windows whatever its size, so the buffer is larger there to make fewer of them.
   */
#if defined(_WIN32)
  static constexpr size_t default_write_buffer_size{256 * 1024};
#else
  static constexpr size_t default_write_buffer_size{64 * 1024};
#endif

private:
  std::string _open_mode{'a'};
  std::string _append_filename_format_pattern;
  std::string _file_header;
  size_t _write_buffer_size{default_write_buffer_size};
  std::chrono::milliseconds _minimum_fsync_interval{0};
  std::optional<PatternFormatterOptions> _override_pattern_formatter_options;
  Timezone _time_zone{Timezone::LocalTime};
  FilenameAppendOption _filename_append_option{FilenameAppendOption::None};
  bool _fsync_enabled{false};
};

namespace detail
{
/** Formats `timestamp_ns` with a strftime pattern, for use in file names */
QUILL_NODISCARD inline std::string format_datetime_string(uint64_t timestamp_ns, Timezone time_zone,
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

  return detail::format_strftime(append_format_pattern, now_tm, time_zone);
}

/** Inserts the formatted timestamp between the file stem and its extension */
QUILL_NODISCARD inline fs::path append_datetime_to_filename(fs::path const& filename,
                                                            std::string const& append_filename_format_pattern,
                                                            Timezone time_zone,
                                                            std::chrono::system_clock::time_point timestamp)
{
  uint64_t const timestamp_ns = static_cast<uint64_t>(
    std::chrono::duration_cast<std::chrono::nanoseconds>(timestamp.time_since_epoch()).count());

  // Compose as paths rather than strings so non-ASCII names round-trip on every platform
  fs::path result = filename.parent_path() / filename.stem();
  result += format_datetime_string(timestamp_ns, time_zone, append_filename_format_pattern);
  result += filename.extension();
  return result;
}

/** The file name with the configured start timestamp appended, if any */
QUILL_NODISCARD inline fs::path get_updated_filename_with_appended_datetime(
  fs::path const& filename, FilenameAppendOption append_to_filename_option,
  std::string const& append_filename_format_pattern, Timezone time_zone,
  std::chrono::system_clock::time_point timestamp)
{
  if ((append_to_filename_option == FilenameAppendOption::None) || (filename == "/dev/null"))
  {
    return filename;
  }

  return append_datetime_to_filename(filename, append_filename_format_pattern, time_zone, timestamp);
}

#if defined(_WIN32)
/** Closes a Windows handle unless it was released */
struct OpenedHandleGuard
{
  ~OpenedHandleGuard()
  {
    if (handle != INVALID_HANDLE_VALUE)
    {
      ::CloseHandle(handle);
    }
  }

  QUILL_NODISCARD HANDLE release() noexcept
  {
    HANDLE const released_handle = handle;
    handle = INVALID_HANDLE_VALUE;
    return released_handle;
  }

  HANDLE handle{INVALID_HANDLE_VALUE};
};

/**
 * The path of an open file or directory in extended-length form, spelled as the filesystem stores
 * it, e.g. with the stored case of each name. Empty if the filesystem cannot report it.
 */
QUILL_NODISCARD inline fs::path get_final_path(HANDLE handle)
{
  constexpr DWORD flags{FILE_NAME_NORMALIZED | VOLUME_NAME_DOS};
  std::wstring final_path(MAX_PATH, L'\0');
  DWORD length =
    ::GetFinalPathNameByHandleW(handle, final_path.data(), static_cast<DWORD>(final_path.size()), flags);

  if (length >= final_path.size())
  {
    // The buffer is too small, and the length is the size it needs, including the terminator
    final_path.resize(length);
    length =
      ::GetFinalPathNameByHandleW(handle, final_path.data(), static_cast<DWORD>(final_path.size()), flags);
  }

  if ((length == 0) || (length >= final_path.size()))
  {
    return fs::path{};
  }

  final_path.resize(length);
  return fs::path{final_path};
}
#endif

/**
 * fs::canonical() of an existing path, spelled as the filesystem stores it with every standard
 * library: MinGW's fs::canonical() keeps the requested case of each name. On Windows the result
 * is in extended-length form. Empty on failure.
 */
QUILL_NODISCARD inline fs::path get_canonical_path(fs::path const& path)
{
#if defined(_WIN32)
  OpenedHandleGuard const file{::CreateFileW(
    path.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr)};
  return (file.handle == INVALID_HANDLE_VALUE) ? fs::path{} : get_final_path(file.handle);
#else
  std::error_code ec;
  return fs::canonical(path, ec);
#endif
}
} // namespace detail

/*
 * The Windows and POSIX FileSink classes are kept separate, sharing only the free functions
 * above. A previous attempt to share a base class across all platforms introduced a measurable
 * Linux throughput regression in BENCHMARK_quill_backend_throughput despite equivalent logic.
 * If this is revisited, revalidate Linux performance.
 */

#if defined(_WIN32)
/**
 * Windows FileSink implementation.
 * Uses a native HANDLE + WriteFile path because it provides materially better
 * backend file-write throughput on Windows than the CRT FILE* path.
 */
class FileSink : public StreamSink
{
private:
  friend class detail::SinkManager;

  static std::string _file_sink_id(fs::path const& normalized_path)
  {
    fs::path path{normalized_path};
    if (path.is_absolute() || detail::is_extended_windows_path(path))
    {
      detail::OpenedHandleGuard const directory{::CreateFileW(
        path.parent_path().c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr)};
      if (directory.handle == INVALID_HANDLE_VALUE)
      {
        QUILL_THROW(QuillError{"Cannot inspect file sink directory. GetLastError: " +
                               std::to_string(::GetLastError())});
      }

      // FileCaseSensitiveInfo (23) has a single ULONG flag; older SDKs omit the declarations.
      DWORD flags{0};
      bool const queried =
        ::GetFileInformationByHandleEx(directory.handle, static_cast<FILE_INFO_BY_HANDLE_CLASS>(23),
                                       &flags, sizeof(flags)) != 0;
      DWORD const error = queried ? ERROR_SUCCESS : ::GetLastError();
      if (!queried && error != ERROR_INVALID_PARAMETER && error != ERROR_NOT_SUPPORTED && error != ERROR_INVALID_FUNCTION)
      {
        QUILL_THROW(QuillError{"Cannot query file sink directory case sensitivity. GetLastError: " +
                               std::to_string(error)});
      }

      // Spell the directory as the filesystem stores it, so that its case aliases share the key:
      // MinGW's fs::canonical() keeps the requested case in the normalized path. A filesystem
      // that cannot report the stored path keeps the normalized spelling.
      fs::path const stored_directory = detail::get_final_path(directory.handle);
      if (!stored_directory.empty())
      {
        path = stored_directory / path.filename();
      }

      if ((flags & 1u) == 0)
      {
        std::wstring const name = path.filename().native();
        int const count = ::LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_UPPERCASE, name.c_str(),
                                          static_cast<int>(name.size()), nullptr, 0, nullptr, nullptr, 0);
        if (count == 0)
        {
          QUILL_THROW(QuillError{"Cannot normalize file sink name. GetLastError: " +
                                 std::to_string(::GetLastError())});
        }

        std::wstring folded(static_cast<size_t>(count), L'\0');
        int const mapped_count =
          ::LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_UPPERCASE, name.c_str(),
                          static_cast<int>(name.size()), folded.data(), count, nullptr, nullptr, 0);
        if (mapped_count == 0)
        {
          QUILL_THROW(QuillError{"Cannot normalize file sink name. GetLastError: " +
                                 std::to_string(::GetLastError())});
        }

        path = path.parent_path() / folded;
      }

      // Ordinary and extended paths share a registry key; the actual I/O path is unchanged.
      // Keep the prefix for trailing dots/spaces, whose filename meaning depends on it.
      wchar_t const last_char = path.native().back();
      if ((last_char != L'.') && (last_char != L' '))
      {
        path = detail::to_ordinary_windows_path(path);
      }
    }

    return detail::file_sink_path_string(path);
  }

public:
  explicit FileSink(fs::path const& filename, FileSinkConfig const& config = FileSinkConfig{},
                    FileEventNotifier file_event_notifier = FileEventNotifier{}, bool do_fopen = true,
                    std::chrono::system_clock::time_point start_time = std::chrono::system_clock::now())
    : StreamSink(detail::get_updated_filename_with_appended_datetime(
                   filename, config.filename_append_option(),
                   config.append_filename_format_pattern(), config.timezone(), start_time),
                 nullptr, config.override_pattern_formatter_options(), std::move(file_event_notifier)),
      _config(config)
  {
    if (!_config.fsync_enabled() && (_config.minimum_fsync_interval().count() != 0))
    {
      QUILL_THROW(
        QuillError{"Cannot set a non-zero minimum fsync interval when fsync is disabled."});
    }

    if (do_fopen && !is_null())
    {
      open_file(_filename, _config.open_mode());
    }
  }

  ~FileSink() override
  {
    _fsync_pending_noexcept();
    _close_file_noexcept();
  }

  /**
   * @return the configuration that the sink was created with. A sink that is retrieved for a
   * file keeps the configuration of the call that created it.
   */
  QUILL_NODISCARD FileSinkConfig const& config() const noexcept { return _config; }

  /**
   * Flushes buffered records to the file and fsyncs it when enabled. The final flush also
   * performs an fsync deferred by minimum_fsync_interval.
   */
  QUILL_ATTRIBUTE_HOT void flush_sink(SinkFlushReason flush_reason) override
  {
    bool const force_fsync = (flush_reason == SinkFlushReason::Final);

    if (_write_occurred && !is_open())
    {
      _reopen_file();
    }

    if (!_write_occurred)
    {
      // Nothing new to write, but an fsync skipped by minimum_fsync_interval may still be owed
      if (_fsync_pending && _config.fsync_enabled())
      {
        fsync_file(force_fsync);
      }

      return;
    }

    _flush_native_write_buffer();
    _write_occurred = false;

    if (_config.fsync_enabled())
    {
      fsync_file(force_fsync);
    }

    std::error_code ec;
    bool const file_exists = fs::exists(_filename, ec);
    bool file_removed = !ec && !file_exists;
    if (ec)
    {
      // libc++ can report access denied for a file pending deletion. Query the open
      // handle so an unrelated status error does not cause a valid file to be reopened.
      FILE_STANDARD_INFO file_info{};
      file_removed = ::GetFileInformationByHandleEx(_native_file_handle, FileStandardInfo,
                                                    &file_info, sizeof(file_info)) &&
        file_info.DeletePending;
    }
    if (file_removed)
    {
      // Keep recovery pending even when a close callback throws after closing the handle.
      _write_occurred = true;
      close_file();
      _reopen_file();
    }
  }

  QUILL_ATTRIBUTE_HOT void write_log(LogRecord const& record) override
  {
    if (QUILL_UNLIKELY(_native_file_handle == INVALID_HANDLE_VALUE))
    {
      return;
    }

    std::string_view const log_statement = record.log_statement;
    std::string_view statement = log_statement;
    std::string user_log_statement;

    if (_file_event_notifier.before_write)
    {
      user_log_statement = _file_event_notifier.before_write(log_statement);
      statement = user_log_statement;
    }

    auto const stmt_size = statement.size();

    if (_native_write_pos + stmt_size > _native_write_buffer_cap)
    {
      _flush_native_write_buffer();
    }

    if (QUILL_LIKELY(stmt_size <= _native_write_buffer_cap))
    {
      std::memcpy(_native_write_buffer.get() + _native_write_pos, statement.data(), stmt_size);
      _native_write_pos += stmt_size;
    }
    else
    {
      _write_to_file(statement.data(), stmt_size);
    }

    _file_size += stmt_size;
    _write_occurred = true;
  }

protected:
  void _reopen_file()
  {
    open_file(_filename, _config.open_mode());
    std::error_code ec;
    uint64_t const reopened_file_size = fs::file_size(_filename, ec);
    _file_size = ec ? 0 : reopened_file_size;
    _write_occurred = false;
  }

  void open_file(fs::path const& filename, std::string const& mode)
  {
    if (_file_event_notifier.before_open)
    {
      _file_event_notifier.before_open(filename);
    }

    // Retry only while another process holds the file, e.g. an antivirus scan. Other errors such
    // as a missing directory are not transient and must not stall the backend thread.
    constexpr int max_attempts{3};
    constexpr uint64_t retry_delay_ns{200ull * 1'000'000ull};
    DWORD const creation_disposition = (!mode.empty() && (mode[0] == 'w')) ? CREATE_ALWAYS : OPEN_ALWAYS;
    detail::OpenedHandleGuard opened_handle_guard;

    for (int attempt{1};; ++attempt)
    {
      opened_handle_guard.handle = ::CreateFileW(
        filename.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, creation_disposition, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);

      if (opened_handle_guard.handle != INVALID_HANDLE_VALUE)
      {
        break;
      }

      DWORD const last_error = ::GetLastError();
      bool const is_transient = (last_error == ERROR_SHARING_VIOLATION) || (last_error == ERROR_LOCK_VIOLATION);

      if (!is_transient || (attempt == max_attempts))
      {
        QUILL_THROW(QuillError{std::string{"CreateFileW failed after "} + std::to_string(attempt) +
                               " attempt(s), path: " + detail::file_sink_path_string(filename) +
                               " mode: " + mode + " GetLastError: " + std::to_string(last_error)});
      }

      detail::sleep_for_ns(retry_delay_ns);
    }

    _native_write_buffer_cap = _native_write_buffer_capacity();
    _native_write_buffer = std::make_unique<char[]>(_native_write_buffer_cap);
    _native_write_pos = 0;

    bool const append_mode = !mode.empty() && mode[0] == 'a';

    // A file that is empty gets its start here: what after_open writes to it, and the header
    bool const is_new_file = _is_empty_file(opened_handle_guard.handle);

    if (_file_event_notifier.after_open)
    {
      if (append_mode && ::GetFileType(opened_handle_guard.handle) == FILE_TYPE_DISK)
      {
        LARGE_INTEGER offset{};
        if (!::SetFilePointerEx(opened_handle_guard.handle, offset, nullptr, FILE_END))
        {
          DWORD const seek_error = ::GetLastError();
          QUILL_THROW(QuillError{"SetFilePointerEx failed. GetLastError: " + std::to_string(seek_error)});
        }
      }

      // Called with the file as it was opened, before the sink writes to it
      _file_event_notifier.after_open(filename, opened_handle_guard.handle);
    }

    if (is_new_file && !_config.file_header().empty())
    {
      // Written to the file rather than to the write buffer, so that the size of the file
      // includes it
      std::string_view file_header = _config.file_header();
      std::string user_file_header;

      if (_file_event_notifier.before_write)
      {
        user_file_header = _file_event_notifier.before_write(file_header);
        file_header = user_file_header;
      }

      _write_to_handle(opened_handle_guard.handle, append_mode, file_header.data(), file_header.size());
    }

    uint64_t const initial_file_size = is_new_file ? _opened_file_size(opened_handle_guard.handle) : 0;

    // A subclass may reopen a different path. Commit it only after the open and callbacks succeed.
    if (filename != _filename)
    {
      _filename = detail::normalize_file_sink_path(filename, false);
    }

    _append_mode = append_mode;
    _native_file_handle = opened_handle_guard.release();
    _set_initial_file_size(initial_file_size);
  }

  void close_file()
  {
    if (_native_file_handle == INVALID_HANDLE_VALUE)
    {
      return;
    }

    if (_file_event_notifier.before_close)
    {
      // Callbacks write through the handle, outside Quill's separate native buffer.
      _flush_native_write_buffer();
      QUILL_TRY { _file_event_notifier.before_close(_filename, _native_file_handle); }
  #if !defined(QUILL_NO_EXCEPTIONS)
      QUILL_CATCH_ALL()
      {
        HANDLE native_file_handle = _native_file_handle;
        QUILL_TRY { _flush_native_write_buffer(); }
        QUILL_CATCH_ALL() {}
        _native_file_handle = INVALID_HANDLE_VALUE;
        _append_mode = false;
        _native_write_pos = 0;
        ::CloseHandle(native_file_handle);
        throw;
      }
  #endif
    }

    _flush_native_write_buffer();
    ::CloseHandle(_native_file_handle);
    _native_file_handle = INVALID_HANDLE_VALUE;
    _append_mode = false;
    _native_write_pos = 0;

    if (_file_event_notifier.after_close)
    {
      _file_event_notifier.after_close(_filename);
    }
  }

  /**
   * Writes buffered records and syncs the open file, for a sink destroyed without a final flush,
   * e.g. when its last logger is removed. Only the existing handle is used: unlike flush_sink(),
   * this never reopens or recreates the file at its path.
   */
  void _fsync_pending_noexcept() noexcept
  {
    if (!_config.fsync_enabled() || !is_open() || (!_fsync_pending && !_write_occurred))
    {
      return;
    }

    QUILL_TRY
    {
      _flush_native_write_buffer();
      fsync_file(true);
    }
  #if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH_ALL() {}
  #endif
  }

  void _close_file_noexcept() noexcept
  {
    QUILL_TRY { close_file(); }
  #if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH_ALL()
    {
      if (_native_file_handle != INVALID_HANDLE_VALUE)
      {
        ::CloseHandle(_native_file_handle);
        _native_file_handle = INVALID_HANDLE_VALUE;
      }
    }
  #endif
  }

  void fsync_file(bool force_fsync = false)
  {
    if (_native_file_handle == INVALID_HANDLE_VALUE)
    {
      return;
    }

    // A forced fsync (rotation, the final flush) ignores minimum_fsync_interval
    auto const now = std::chrono::steady_clock::now();

    if (!force_fsync && _is_within_minimum_fsync_interval(now))
    {
      // Deferred: a later flush performs it once the interval has elapsed, or the final flush
      _fsync_pending = true;
      return;
    }

    if (!::FlushFileBuffers(_native_file_handle))
    {
      _write_occurred = true;
      QUILL_THROW(QuillError{std::string{"FlushFileBuffers failed. GetLastError: "} +
                             std::to_string(::GetLastError())});
    }

    _fsync_pending = false;

    if (!force_fsync)
    {
      _last_fsync_timestamp = now;
    }
  }

  /** True if the previous fsync was less than minimum_fsync_interval before `now` */
  QUILL_NODISCARD bool _is_within_minimum_fsync_interval(std::chrono::steady_clock::time_point now) const noexcept
  {
    // Before the first fsync there is no interval to wait for. Measuring from the clock's epoch
    // would defer the first fsync whenever the system has been up for less than the interval
    bool const fsynced_before = (_last_fsync_timestamp != std::chrono::steady_clock::time_point{});
    return fsynced_before && ((now - _last_fsync_timestamp) < _config.minimum_fsync_interval());
  }

  QUILL_NODISCARD bool is_open() const noexcept
  {
    return _native_file_handle != INVALID_HANDLE_VALUE;
  }

  /**
   * Keeps the size that a file had when open_file() had created it: its header and what
   * after_open wrote to it. It is 0 for a file that already had content. These bytes were written
   * past the write buffer, so they are still to be synced when fsync is enabled.
   */
  void _set_initial_file_size(uint64_t initial_file_size) noexcept
  {
    _initial_file_size = initial_file_size;

    if ((initial_file_size != 0) && _config.fsync_enabled())
    {
      _fsync_pending = true;
    }
  }

  /** @return true if nothing was written to the open file yet. A pipe or a device has no size */
  QUILL_NODISCARD static bool _is_empty_file(HANDLE file_handle) noexcept
  {
    if (::GetFileType(file_handle) != FILE_TYPE_DISK)
    {
      return true;
    }

    LARGE_INTEGER size{};
    return ::GetFileSizeEx(file_handle, &size) && (size.QuadPart == 0);
  }

  /** @return the size of the open file, 0 for a pipe or a device */
  QUILL_NODISCARD static uint64_t _opened_file_size(HANDLE file_handle) noexcept
  {
    if (::GetFileType(file_handle) != FILE_TYPE_DISK)
    {
      return 0;
    }

    LARGE_INTEGER size{};
    return ::GetFileSizeEx(file_handle, &size) ? static_cast<uint64_t>(size.QuadPart) : 0;
  }

  void _write_to_file(char const* data, size_t size)
  {
    _write_to_handle(_native_file_handle, _append_mode, data, size);
  }

  static void _write_to_handle(HANDLE file_handle, bool append_mode, char const* data, size_t size)
  {
    while (size != 0)
    {
      DWORD bytes_written = 0;
      constexpr size_t max_dword = static_cast<size_t>(~DWORD{0});
      DWORD const chunk = static_cast<DWORD>(std::min<size_t>(size, max_dword));
      OVERLAPPED overlapped{};
      OVERLAPPED* overlapped_ptr{nullptr};

      if (append_mode)
      {
        overlapped.Offset = 0xFFFFFFFF;
        overlapped.OffsetHigh = 0xFFFFFFFF;
        overlapped_ptr = &overlapped;
      }

      if (!::WriteFile(file_handle, data, chunk, &bytes_written, overlapped_ptr) || (bytes_written == 0))
      {
        QUILL_THROW(QuillError{std::string{"WriteFile failed. GetLastError: "} +
                               std::to_string(::GetLastError())});
      }

      data += bytes_written;
      size -= bytes_written;
    }
  }

  void _flush_native_write_buffer()
  {
    if ((_native_file_handle == INVALID_HANDLE_VALUE) || (_native_write_pos == 0))
    {
      return;
    }

    _write_to_file(_native_write_buffer.get(), _native_write_pos);
    _native_write_pos = 0;
  }

  QUILL_NODISCARD size_t _native_write_buffer_capacity() const noexcept
  {
    return _config.write_buffer_size() == 0 ? default_write_buffer_size : _config.write_buffer_size();
  }

protected:
  static constexpr size_t default_write_buffer_size{FileSinkConfig::default_write_buffer_size};
  FileSinkConfig _config;
  std::chrono::steady_clock::time_point _last_fsync_timestamp{};
  uint64_t _initial_file_size{0}; /**< Size of the open file when open_file() had created it, 0 if it had content */
  bool _fsync_pending{false}; /**< An fsync was skipped because of minimum_fsync_interval, or a header is not synced */
  HANDLE _native_file_handle{INVALID_HANDLE_VALUE};
  std::unique_ptr<char[]> _native_write_buffer;
  size_t _native_write_pos{0};
  size_t _native_write_buffer_cap{0};
  bool _append_mode{false};
};
#else
/**
 * FileSink
 * Writes the log messages to a file
 */
class FileSink : public StreamSink
{
public:
  explicit FileSink(fs::path const& filename, FileSinkConfig const& config = FileSinkConfig{},
                    FileEventNotifier file_event_notifier = FileEventNotifier{}, bool do_fopen = true,
                    std::chrono::system_clock::time_point start_time = std::chrono::system_clock::now())
    : StreamSink(detail::get_updated_filename_with_appended_datetime(
                   filename, config.filename_append_option(),
                   config.append_filename_format_pattern(), config.timezone(), start_time),
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

  ~FileSink() override
  {
    _fsync_pending_noexcept();
    _close_file_noexcept();
  }

  /**
   * @return the configuration that the sink was created with. A sink that is retrieved for a
   * file keeps the configuration of the call that created it.
   */
  QUILL_NODISCARD FileSinkConfig const& config() const noexcept { return _config; }

  /**
   * Flushes buffered records to the file and fsyncs it when enabled. The final flush also
   * performs an fsync deferred by minimum_fsync_interval.
   */
  QUILL_ATTRIBUTE_HOT void flush_sink(SinkFlushReason flush_reason) override
  {
    bool const force_fsync = (flush_reason == SinkFlushReason::Final);

    if (_write_occurred && !is_open())
    {
      _reopen_file();
    }

    if (!_file)
    {
      return;
    }

    if (!_write_occurred)
    {
      // Nothing new to write, but an fsync skipped by minimum_fsync_interval may still be owed
      if (_fsync_pending && _config.fsync_enabled())
      {
        fsync_file(force_fsync);
      }

      return;
    }

    StreamSink::flush_sink(flush_reason);

    if (_config.fsync_enabled())
    {
      fsync_file(force_fsync);
    }

    std::error_code ec;
    bool const file_exists = fs::exists(_filename, ec);
    if (!ec && !file_exists)
    {
      // Keep recovery pending even when a close callback throws after closing the handle.
      _write_occurred = true;
      close_file();
      _reopen_file();
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
  void _reopen_file()
  {
    open_file(_filename, _config.open_mode());
    std::error_code ec;
    uint64_t const reopened_file_size = fs::file_size(_filename, ec);
    _file_size = ec ? 0 : reopened_file_size;
    _write_occurred = false;
  }

  void open_file(fs::path const& filename, std::string const& mode)
  {
    if (_file_event_notifier.before_open)
    {
      _file_event_notifier.before_open(filename);
    }

    std::unique_ptr<char[]> write_buffer;
    OpenedFileGuard opened_file_guard;

    int flags = O_CREAT | O_WRONLY | O_CLOEXEC;
    flags |= (!mode.empty() && mode[0] == 'w') ? O_TRUNC : O_APPEND;

    // Retry only on EINTR. Other errors such as a missing directory are not transient, and
    // sleeping between attempts would stall the backend thread on every recovery attempt.
    int fd{-1};
    do
    {
      // Let the process umask decide the permissions, as fopen() does.
      fd = ::open(filename.string().data(), flags, 0666);
    } while (fd == -1 && errno == EINTR);

    if (fd != -1)
    {
      opened_file_guard.file = ::fdopen(fd, mode.data());
      if (!opened_file_guard.file)
      {
        int const fdopen_errno = errno;
        ::close(fd);
        errno = fdopen_errno;
      }
    }

    if (!opened_file_guard.file)
    {
      // Building the message can change errno
      int const open_errno = errno;
      QUILL_THROW(QuillError{std::string{"fopen failed, path: "} + filename.string() + " mode: " + mode +
                             " errno: " + std::to_string(open_errno) + " error: " + std::strerror(open_errno)});
    }

    if (_config.write_buffer_size() != 0)
    {
      write_buffer = std::make_unique<char[]>(_config.write_buffer_size());

      if (setvbuf(opened_file_guard.file, write_buffer.get(), _IOFBF, _config.write_buffer_size()) != 0)
      {
        QUILL_THROW(QuillError{std::string{"setvbuf failed error: "} + std::strerror(errno)});
      }
    }

    // A file that is empty gets its start here: what after_open writes to it, and the header
    bool const is_new_file = _is_empty_file(opened_file_guard.file);

    if (_file_event_notifier.after_open)
    {
      // Called with the file as it was opened, before the sink writes to it, so the callback can
      // still configure the stream
      _file_event_notifier.after_open(filename, opened_file_guard.file);

      // What the callback wrote to the stream is part of the size of the file
      _flush_opened_file(opened_file_guard.file, filename);
    }

    if (is_new_file && !_config.file_header().empty())
    {
      _write_file_header(opened_file_guard.file, filename);
    }

    uint64_t const initial_file_size = is_new_file ? _opened_file_size(opened_file_guard.file) : 0;

    // A subclass may reopen a different path. Commit it only after the open and callbacks succeed.
    if (filename != _filename)
    {
      _filename = detail::normalize_file_sink_path(filename, false);
    }

    _file = opened_file_guard.release();
    _write_buffer = std::move(write_buffer);
    _set_initial_file_size(initial_file_size);
  }

  void close_file()
  {
    if (!_file)
    {
      return;
    }

    if (_file_event_notifier.before_close)
    {
      QUILL_TRY { _file_event_notifier.before_close(_filename, _file); }
  #if !defined(QUILL_NO_EXCEPTIONS)
      QUILL_CATCH_ALL()
      {
        FILE* file = _file;
        _file = nullptr;
        std::fclose(file);
        throw;
      }
  #endif
    }

    FILE* file = _file;
    _file = nullptr;

    if (std::fclose(file) != 0)
    {
      int const saved_errno = errno;
      QUILL_THROW(QuillError{std::string{"fclose failed errno: "} + std::to_string(saved_errno) +
                             " error: " + std::strerror(saved_errno)});
    }

    if (_file_event_notifier.after_close)
    {
      _file_event_notifier.after_close(_filename);
    }
  }

  /**
   * Writes buffered records and syncs the open file, for a sink destroyed without a final flush,
   * e.g. when its last logger is removed. Only the existing handle is used: unlike flush_sink(),
   * this never reopens or recreates the file at its path.
   */
  void _fsync_pending_noexcept() noexcept
  {
    if (!_config.fsync_enabled() || !is_open() || (!_fsync_pending && !_write_occurred))
    {
      return;
    }

    QUILL_TRY
    {
      StreamSink::flush_sink(SinkFlushReason::Final);
      fsync_file(true);
    }
  #if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH_ALL() {}
  #endif
  }

  void _close_file_noexcept() noexcept
  {
    QUILL_TRY { close_file(); }
  #if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH_ALL() {}
  #endif
  }

  void fsync_file(bool force_fsync = false)
  {
    if (!_file)
    {
      return;
    }

    // A forced fsync (rotation, the final flush) ignores minimum_fsync_interval
    auto const now = std::chrono::steady_clock::now();

    if (!force_fsync && _is_within_minimum_fsync_interval(now))
    {
      // Deferred: a later flush performs it once the interval has elapsed, or the final flush
      _fsync_pending = true;
      return;
    }

    // Retry on EINTR — a signal delivered during fsync() causes it to fail transiently.
    int ret{0};
    do
    {
      ret = ::fsync(fileno(_file));
    } while (ret != 0 && errno == EINTR);

    if (QUILL_UNLIKELY(ret != 0))
    {
      int const saved_errno = errno;
      _write_occurred = true;
      QUILL_THROW(QuillError{std::string{"fsync failed errno: "} + std::to_string(saved_errno) +
                             " error: " + std::strerror(saved_errno)});
    }

    _fsync_pending = false;

    if (!force_fsync)
    {
      _last_fsync_timestamp = now;
    }
  }

  /** True if the previous fsync was less than minimum_fsync_interval before `now` */
  QUILL_NODISCARD bool _is_within_minimum_fsync_interval(std::chrono::steady_clock::time_point now) const noexcept
  {
    // Before the first fsync there is no interval to wait for. Measuring from the clock's epoch
    // would defer the first fsync whenever the system has been up for less than the interval
    bool const fsynced_before = (_last_fsync_timestamp != std::chrono::steady_clock::time_point{});
    return fsynced_before && ((now - _last_fsync_timestamp) < _config.minimum_fsync_interval());
  }

  QUILL_NODISCARD bool is_open() const noexcept { return _file != nullptr; }

  /**
   * Keeps the size that a file had when open_file() had created it: its header and what
   * after_open wrote to it. It is 0 for a file that already had content. These bytes were written
   * past the write buffer, so they are still to be synced when fsync is enabled.
   */
  void _set_initial_file_size(uint64_t initial_file_size) noexcept
  {
    _initial_file_size = initial_file_size;

    if ((initial_file_size != 0) && _config.fsync_enabled())
    {
      _fsync_pending = true;
    }
  }

  /** @return true if nothing was written to the open file yet. A pipe or a device has no size */
  QUILL_NODISCARD static bool _is_empty_file(FILE* file) noexcept
  {
    struct stat file_stat
    {
    };
    return (::fstat(fileno(file), &file_stat) == 0) && (file_stat.st_size == 0);
  }

  /** @return the size of the open file, without what is in its write buffer */
  QUILL_NODISCARD static uint64_t _opened_file_size(FILE* file) noexcept
  {
    struct stat file_stat
    {
    };
    return (::fstat(fileno(file), &file_stat) == 0) ? static_cast<uint64_t>(file_stat.st_size) : 0;
  }

  /**
   * Writes the write buffer of a file that open_file() is opening. A flush that fails is not
   * tried again, also when it was interrupted: the stream can have discarded what it had buffered,
   * and the next flush would then report success for bytes that are not in the file.
   */
  static void _flush_opened_file(FILE* file, fs::path const& filename)
  {
    if (std::fflush(file) != 0)
    {
      int const flush_errno = errno;
      QUILL_THROW(QuillError{std::string{"fflush failed, path: "} + filename.string() + " errno: " +
                             std::to_string(flush_errno) + " error: " + std::strerror(flush_errno)});
    }
  }

  /**
   * Writes the header of a new file, see FileSinkConfig::set_file_header(). It goes to the file
   * descriptor, past the stream, which has nothing buffered at this point: each write tells how
   * many bytes are in the file, so an interrupted or partial write is continued until the header
   * is complete, and the size of the file includes it.
   */
  void _write_file_header(FILE* file, fs::path const& filename)
  {
    std::string_view file_header = _config.file_header();
    std::string user_file_header;

    if (_file_event_notifier.before_write)
    {
      user_file_header = _file_event_notifier.before_write(file_header);
      file_header = user_file_header;
    }

    int const file_descriptor = fileno(file);
    size_t bytes_written{0};

    while (bytes_written < file_header.size())
    {
      ssize_t const written =
        ::write(file_descriptor, file_header.data() + bytes_written, file_header.size() - bytes_written);

      if (written > 0)
      {
        bytes_written += static_cast<size_t>(written);
        continue;
      }

      // Nothing was written by an interrupted call, so it is made again
      if ((written < 0) && (errno == EINTR))
      {
        continue;
      }

      int const write_errno = errno;
      QUILL_THROW(QuillError{std::string{"Failed to write the file header, path: "} + filename.string() +
                             " errno: " + std::to_string(write_errno) + " error: " + std::strerror(write_errno)});
    }
  }

protected:
  FileSinkConfig _config;
  std::chrono::steady_clock::time_point _last_fsync_timestamp{};
  uint64_t _initial_file_size{0}; /**< Size of the open file when open_file() had created it, 0 if it had content */
  bool _fsync_pending{false}; /**< An fsync was skipped because of minimum_fsync_interval, or a header is not synced */
  std::unique_ptr<char[]> _write_buffer;
};
#endif

QUILL_END_EXPORT

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
  #pragma warning(pop)
#endif

QUILL_END_NAMESPACE
