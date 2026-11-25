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
  #include <cstddef>
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
 * What a file sink writes before the bytes of a binary record, see
 * FileSinkConfig::set_binary_record_framing(). The fields are in the byte order of the machine.
 */
enum class BinaryRecordFraming : uint8_t
{
  /** Nothing: the file has the bytes of the records, one after the other */
  None,

  /** The number of bytes of the record, as a uint32_t */
  Size,

  /**
   * The timestamp of the record in nanoseconds since epoch, as a uint64_t, and then the number of
   * bytes of the record, as a uint32_t
   */
  TimestampAndSize
};

namespace detail
{
/** The most bytes that a file sink writes before a binary record */
constexpr size_t max_binary_record_frame_size{sizeof(uint64_t) + sizeof(uint32_t)};

/** @return the number of bytes that a file sink writes before each binary record */
QUILL_NODISCARD constexpr size_t binary_record_frame_size(BinaryRecordFraming framing) noexcept
{
  return (framing == BinaryRecordFraming::None)
    ? 0u
    : ((framing == BinaryRecordFraming::Size) ? sizeof(uint32_t) : max_binary_record_frame_size);
}

/**
 * Writes what precedes the bytes of a binary record to `frame`, which has
 * max_binary_record_frame_size bytes
 * @param timestamp the timestamp of the record
 * @param record_size the number of bytes of the record as they are written. 32 bits count it:
 * the frontend rejects a larger record, see Logger::max_binary_record_size
 * @return the number of bytes written to `frame`
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline size_t make_binary_record_frame(char* frame, BinaryRecordFraming framing,
                                                                           uint64_t timestamp,
                                                                           size_t record_size) noexcept
{
  char* position = frame;

  if (framing == BinaryRecordFraming::TimestampAndSize)
  {
    std::memcpy(position, &timestamp, sizeof(timestamp));
    position += sizeof(timestamp);
  }

  uint32_t const size = static_cast<uint32_t>(record_size);
  std::memcpy(position, &size, sizeof(size));
  position += sizeof(size);

  return static_cast<size_t>(position - frame);
}
} // namespace detail

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
    if (value == FilenameAppendOption::StartCustomTimestampFormat)
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
    else if (value == FilenameAppendOption::StartDateTime)
    {
      _append_filename_format_pattern = "_%Y%m%d_%H%M%S";
    }
    else if (value == FilenameAppendOption::StartDate)
    {
      _append_filename_format_pattern = "_%Y%m%d";
    }

    // Set last, so that the configuration stays as it was when the pattern is rejected
    _filename_append_option = value;
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
   * @note `/dev/null` cannot be synced. With fsync enabled, a sink of `/dev/null` on POSIX reports
   * through BackendOptions::error_notifier that fsync failed. On Windows it opens no file, so
   * there is nothing to sync.
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
   * @brief Sets the size of the write buffer of the sink.
   *
   * The sink copies each record to this buffer, and writes the buffer to the file when it is
   * full and when the backend flushes the sink. A record that is larger than the buffer is
   * written to the file on its own.
   *
   * The sink writes its buffer to the file past the handle that FileEventNotifier::after_open
   * and FileEventNotifier::before_close receive. What a callback writes through the handle stays
   * in order with the records, but the records do not follow how the callback configures the
   * handle, e.g. with `setvbuf`, and the handle is not to be used outside the callback.
   *
   * A size of 0 disables the buffer. The sink then writes every record through that handle as it
   * arrives, so the callbacks decide how the file is buffered and can keep the handle. On Windows
   * each record is then written to the file at once. On other platforms the handle is a `FILE*`,
   * which has the default buffering of the C library, or none with a binary record framing other
   * than BinaryRecordFraming::None, until `after_open` changes it with `setvbuf`. When a write
   * fails, the rest of the record is kept for the next flush if the `FILE*` is unbuffered; a
   * buffered one can discard what it holds.
   *
   * @note By default, a buffer of 64 KiB is used, or of 256 KiB on Windows.
   * @param value Size of the buffer in bytes, 4096 at least, or 0 to disable the buffer.
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
   * A header that cannot be written in full, e.g. to a full disk, does not stay in the file: the
   * file is emptied again, so that it gets its whole header when the sink opens it the next time.
   *
   * @param file_header The header, or empty for none.
   */
  QUILL_ATTRIBUTE_COLD void set_file_header(std::string_view file_header)
  {
    _file_header = file_header;
  }

  /**
   * @brief Sets what the sink writes before the bytes of each binary record, see LOG_BINARY.
   * The default is BinaryRecordFraming::None: the sink writes the bytes and nothing else.
   *
   * With records of different sizes, BinaryRecordFraming::Size lets a reader find where each one
   * ends, and BinaryRecordFraming::TimestampAndSize also keeps the time of each record. The
   * timestamp is the one of the record in the queue, so the logging thread does nothing more.
   *
   * @param value The framing of the binary records.
   */
  QUILL_ATTRIBUTE_COLD void set_binary_record_framing(BinaryRecordFraming value)
  {
    _binary_record_framing = value;
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
  QUILL_NODISCARD BinaryRecordFraming binary_record_framing() const noexcept
  {
    return _binary_record_framing;
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
  BinaryRecordFraming _binary_record_framing{BinaryRecordFraming::None};
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

#if defined(QUILL_NO_EXCEPTIONS)
/**
 * Discards the failures of QUILL_THROW_OR_RETURN on a thread that has no receiver for them,
 * where a failure would end the process
 */
struct DiscardedErrorsScope
{
  DiscardedErrorsScope() noexcept : is_installed{error_receiver() == nullptr}
  {
    if (is_installed)
    {
      error_receiver() = &receiver;
    }
  }

  ~DiscardedErrorsScope()
  {
    if (is_installed)
    {
      error_receiver() = nullptr;
    }
  }

  static void discard(void*, char const*) noexcept {}

  ErrorReceiver const receiver{&DiscardedErrorsScope::discard, nullptr};
  bool const is_installed;
};
#endif

} // namespace detail

/**
 * FileSink
 *
 * Records are copied to a write buffer of the sink, which is written to the file when it is full
 * and when the sink is flushed. fwrite() locks and unlocks the stream on every call, which for a
 * short record costs more than copying it.
 *
 * On Windows the file is a native HANDLE that is written with WriteFile, because that has
 * materially better backend file-write throughput there than the CRT FILE* path. On POSIX the
 * buffer is written to the file descriptor with write(). The stream of the file is kept for the
 * FileEventNotifier callbacks, and for the records when the write buffer is disabled.
 *
 * The two platforms differ in the functions of the last section of the class only. They share
 * this class and not a base class: a previous attempt to share a base class across all platforms
 * introduced a measurable Linux throughput regression in BENCHMARK_quill_backend_throughput
 * despite equivalent logic. If this is revisited, revalidate Linux performance.
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

    if (do_fopen && _opens_file())
    {
      open_file(_filename, _config.open_mode());
    }
  }

  ~FileSink() override
  {
#if defined(QUILL_NO_EXCEPTIONS)
    // The sink can be destroyed on a thread that has no receiver for a failure, e.g. when the
    // loggers are destroyed as the process exits, or by the application that holds it. What
    // cannot be written or synced by now is given up, as the exceptions of these calls are
    detail::DiscardedErrorsScope const discarded_errors_scope;
#endif

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

    // Set again where the flush fails and returns, which it only does without exceptions
    _flush_failed = false;
    _sync_failed = false;

    if (_write_occurred && !is_open())
    {
      _reopen_file();
    }

    if (!is_open())
    {
      return;
    }

    if (!_write_occurred)
    {
      // Nothing new to write, but an fsync skipped by minimum_fsync_interval may still be owed
      if (_fsync_pending && _config.fsync_enabled())
      {
        _flush_failed = !fsync_file(force_fsync);
      }

      return;
    }

    bool flushed{false};

    QUILL_TRY
    {
      // _flush_stream() clears _write_occurred
      flushed = _flush_native_write_buffer() && _flush_stream();
    }
#if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH_ALL()
    {
      _replace_file_removed_before_failed_flush();
      throw;
    }
#endif

    if (QUILL_UNLIKELY(!flushed))
    {
      // Without exceptions a flush that fails returns. The next flush tries again
      _replace_file_removed_before_failed_flush();
      _flush_failed = true;
      return;
    }

    if (_config.fsync_enabled() && !fsync_file(force_fsync))
    {
      _flush_failed = true;
      return;
    }

    if (_is_file_removed())
    {
      _replace_removed_file();
    }
  }

  QUILL_ATTRIBUTE_HOT void write_log(LogRecord const& record) override
  {
    if (QUILL_UNLIKELY(!is_open()))
    {
      return;
    }

    std::string_view const log_statement = record.log_statement;

    if (_file_event_notifier.before_write)
    {
      std::string const user_log_statement = _file_event_notifier.before_write(log_statement);
      _write_buffered(user_log_statement.data(), user_log_statement.size());
    }
    else
    {
      _write_buffered(log_statement.data(), log_statement.size());
    }
  }

  /**
   * @brief Writes the bytes of a binary record to the file as they are, after the frame that
   * FileSinkConfig::set_binary_record_framing() asks for, which is none by default.
   * FileEventNotifier::before_write transforms the bytes first, as it does a log statement, and
   * the frame has the size of what it returns.
   */
  QUILL_ATTRIBUTE_HOT void write_binary(BinaryRecord const& record) override
  {
    if (QUILL_UNLIKELY(!is_open()))
    {
      return;
    }

    std::string_view const bytes{reinterpret_cast<char const*>(record.data), record.size};

    if (_file_event_notifier.before_write)
    {
      std::string const user_bytes = _file_event_notifier.before_write(bytes);
      _write_binary_record(record.timestamp, user_bytes);
    }
    else
    {
      _write_binary_record(record.timestamp, bytes);
    }
  }

protected:
  /** Writes the bytes of a binary record, after its frame when the configuration asks for one */
  QUILL_ATTRIBUTE_HOT void _write_binary_record(uint64_t timestamp, std::string_view bytes)
  {
    if (_config.binary_record_framing() != BinaryRecordFraming::None)
    {
      char frame[detail::max_binary_record_frame_size];
      size_t const frame_size = detail::make_binary_record_frame(
        frame, _config.binary_record_framing(), timestamp, bytes.size());
      _write_buffered(frame, frame_size, bytes.data(), bytes.size());
      return;
    }

    // A record without bytes has no data to copy from
    if (QUILL_LIKELY(!bytes.empty()))
    {
      _write_buffered(bytes.data(), bytes.size());
    }
  }

  /**
   * Appends two parts that belong together to the native write buffer: the frame of a binary
   * record and its bytes. When the buffer is full and cannot be written to the file, neither is
   * appended, so a frame is not written later without its record.
   */
  QUILL_ATTRIBUTE_HOT void _write_buffered(char const* first, size_t first_size, char const* second, size_t second_size)
  {
    size_t const size = first_size + second_size;

    // Counted also when the flush below fails, see the overload below
    _file_size += size;
    _write_occurred = true;

    if (_native_write_pos + size > _native_write_buffer_cap)
    {
      if (QUILL_UNLIKELY(!_flush_native_write_buffer()))
      {
        // Without exceptions a flush that fails returns, and the buffer has no room for the record
        return;
      }
    }

    if (QUILL_LIKELY(size <= _native_write_buffer_cap))
    {
      std::memcpy(_native_write_buffer.get() + _native_write_pos, first, first_size);

      // A record without bytes has no data to copy from
      if (QUILL_LIKELY(second_size != 0))
      {
        std::memcpy(_native_write_buffer.get() + _native_write_pos + first_size, second, second_size);
      }

      _native_write_pos += size;
      return;
    }

    _write_to_file(first, first_size, second, second_size);
  }

  /** Appends to the native write buffer, which is written to the file when it is full */
  QUILL_ATTRIBUTE_HOT void _write_buffered(char const* data, size_t size)
  {
    // Counted also when the flush below fails: a record that the file cannot take brings a sink
    // that rotates by size closer to its rotation, which makes room when it removes a backup
    _file_size += size;
    _write_occurred = true;

    if (QUILL_UNLIKELY(size == 0))
    {
      return;
    }

    if (_native_write_pos + size > _native_write_buffer_cap)
    {
      if (QUILL_UNLIKELY(!_flush_native_write_buffer()))
      {
        // Without exceptions a flush that fails returns, and the buffer has no room for the record
        return;
      }
    }

    if (QUILL_LIKELY(size <= _native_write_buffer_cap))
    {
      std::memcpy(_native_write_buffer.get() + _native_write_pos, data, size);
      _native_write_pos += size;
    }
    else
    {
      _write_to_file(data, size, nullptr, 0);
    }
  }

  /**
   * A file that was removed, e.g. to make room on a full disk, keeps its space until it is closed.
   * What could not be written to it is lost with it, and the next records go to a new file
   */
  void _replace_file_removed_before_failed_flush()
  {
    if (_is_file_removed())
    {
      _native_write_pos = 0;
      _replace_removed_file();
    }
  }

  /** Closes the open file, which was removed, and opens a new one at its path */
  void _replace_removed_file()
  {
    // Keep recovery pending even when a close callback throws after closing the handle.
    _write_occurred = true;
    close_file();
    _reopen_file();
  }

  void _reopen_file()
  {
    if (!open_file(_filename, _config.open_mode()))
    {
      // Without exceptions an open that fails returns. The next flush tries again
      return;
    }

    std::error_code ec;
    uint64_t const reopened_file_size = fs::file_size(_filename, ec);
    _file_size = ec ? 0 : reopened_file_size;
    _write_occurred = false;
  }

  /** @return false if the file was not opened, which is only returned without exceptions */
  bool open_file(fs::path const& filename, std::string const& mode)
  {
    if (_file_event_notifier.before_open)
    {
      _file_event_notifier.before_open(filename);
    }

    OpenedFileGuard opened_file_guard;

    if (!_open_native_file(opened_file_guard, filename, mode))
    {
      return false;
    }

    // A size of 0 disables the buffer: every record is then written to the file as it arrives
    _native_write_buffer_cap = _config.write_buffer_size();
    _native_write_pos = 0;

    if (_native_write_buffer_cap == 0)
    {
      _native_write_buffer.reset();
    }
    else
    {
      _native_write_buffer = std::make_unique<char[]>(_native_write_buffer_cap);
    }

    // A file that is empty gets its start here: what after_open writes to it, and the header
    bool const is_new_file = _is_empty_file(opened_file_guard.handle);

    if (_file_event_notifier.after_open && !_notify_after_open(opened_file_guard.handle, filename))
    {
      return false;
    }

    if (is_new_file && !_config.file_header().empty() && !_write_file_header(opened_file_guard.handle, filename))
    {
      return false;
    }

    uint64_t const initial_file_size = is_new_file ? _opened_file_size(opened_file_guard.handle) : 0;

    // A subclass may reopen a different path. Commit it only after the open and callbacks succeed.
    if (filename != _filename)
    {
      _filename = detail::normalize_file_sink_path(filename, false);
    }

    _opened_file() = opened_file_guard.release();
    _set_initial_file_size(initial_file_size);
    return true;
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
      if (_flush_native_write_buffer() && _flush_stream())
      {
        fsync_file(true);
      }
    }
#if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH_ALL() {}
#endif
  }

  /** @return false if the file was not synced, which is only returned without exceptions */
  bool fsync_file(bool force_fsync = false)
  {
    if (!is_open())
    {
      return true;
    }

    // A forced fsync (rotation, the final flush) ignores minimum_fsync_interval
    auto const now = std::chrono::steady_clock::now();

    if (!force_fsync && _is_within_minimum_fsync_interval(now))
    {
      // Deferred: a later flush performs it once the interval has elapsed, or the final flush
      _fsync_pending = true;
      return true;
    }

    if (QUILL_UNLIKELY(!_sync_native_file()))
    {
      _write_occurred = true;
      _sync_failed = true;
      QUILL_THROW_OR_RETURN_VALUE(QuillError{_sync_failed_message + _last_error_message()}, false);
    }

    _sync_failed = false;
    _fsync_pending = false;

    if (!force_fsync)
    {
      _last_fsync_timestamp = now;
    }

    return true;
  }

  /** True if the previous fsync was less than minimum_fsync_interval before `now` */
  QUILL_NODISCARD bool _is_within_minimum_fsync_interval(std::chrono::steady_clock::time_point now) const noexcept
  {
    // Before the first fsync there is no interval to wait for. Measuring from the clock's epoch
    // would defer the first fsync whenever the system has been up for less than the interval
    bool const fsynced_before = (_last_fsync_timestamp != std::chrono::steady_clock::time_point{});
    return fsynced_before && ((now - _last_fsync_timestamp) < _config.minimum_fsync_interval());
  }

  /** @return the number of bytes that write_binary() writes for the record, with its frame */
  QUILL_NODISCARD size_t binary_write_size(BinaryRecord const& record) const noexcept
  {
    return detail::binary_record_frame_size(_config.binary_record_framing()) + record.size;
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

  /**
   * Writes the header of a new file, see FileSinkConfig::set_file_header(). It goes to the file
   * rather than to the write buffer, so that the size of the file includes it, and past the
   * stream of a POSIX file, which has nothing buffered at this point.
   * @return false if it was not written, which is only returned without exceptions
   */
  QUILL_NODISCARD bool _write_file_header(FileEventNotifierHandle file_handle, fs::path const& filename)
  {
    std::string_view file_header = _config.file_header();
    std::string user_file_header;

    if (_file_event_notifier.before_write)
    {
      user_file_header = _file_event_notifier.before_write(file_header);
      file_header = user_file_header;
    }

    if (_write_to_native_file(file_handle, file_header.data(), file_header.size()) != file_header.size())
    {
      auto const write_error = _last_error();

      // The file was empty, and is made empty again: with a part of the header in it, the open
      // that follows would take it for a file that has its header, and append the records to that
      bool const is_emptied = _truncate_native_file(file_handle);
      auto const truncate_error = _last_error();

      std::string error_message = std::string{"Failed to write the file header, path: "} +
        detail::file_sink_path_string(filename) + " " + _error_message(write_error);

      if (!is_emptied)
      {
        error_message +=
          ". The part of it that was written is left in the file, " + _error_message(truncate_error);
      }

      QUILL_THROW_OR_RETURN_VALUE(QuillError{error_message}, false);
    }

    return true;
  }

  /** @return false if the file did not take the records, which is only returned without exceptions */
  QUILL_NODISCARD bool _flush_native_write_buffer()
  {
    if (!is_open() || (_native_write_pos == 0))
    {
      return true;
    }

    size_t const bytes_written =
      _write_to_native_file(_opened_file(), _native_write_buffer.get(), _native_write_pos);

    if (QUILL_UNLIKELY(bytes_written != _native_write_pos))
    {
      // Read before anything else can change the error of the write
      auto const last_error = _last_error();

      // Keep what is not in the file for the next flush, so that nothing is written twice. Done
      // before the message is made, which needs memory and can fail to get it
      _native_write_pos -= bytes_written;
      std::memmove(_native_write_buffer.get(), _native_write_buffer.get() + bytes_written, _native_write_pos);

      QUILL_THROW_OR_RETURN_VALUE(QuillError{_write_failed_message + _error_message(last_error)}, false);
    }

    _native_write_pos = 0;

    if (QUILL_UNLIKELY(_native_write_buffer_cap != _config.write_buffer_size()))
    {
      // The buffer grew to keep the rest of a record that a write cut short. Back at its size, the
      // records larger than it, or every record when it is disabled, are written past it again
      _restore_native_write_buffer_size();
    }

    return true;
  }

  /**
   * Gives the write buffer its configured size again. Without the memory for a smaller buffer,
   * the larger one is kept, and used up to the configured size
   */
  void _restore_native_write_buffer_size() noexcept
  {
    _native_write_buffer_cap = _config.write_buffer_size();

    if (_native_write_buffer_cap == 0)
    {
      _native_write_buffer.reset();
      return;
    }

    QUILL_TRY { _native_write_buffer = std::make_unique<char[]>(_native_write_buffer_cap); }
#if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH_ALL() {}
#endif
  }

  /**
   * Keeps in the write buffer what a write past it did not put in the file, growing the buffer if
   * it does not fit. The next flush writes it before any later record, so a binary record keeps
   * its frame. The buffer is empty here: a record is written past it only after it was flushed
   */
  void _keep_unwritten_part(char const* first, size_t first_size, char const* second,
                            size_t second_size, size_t bytes_written)
  {
    size_t const unwritten_size = first_size + second_size - bytes_written;

    if (unwritten_size > _native_write_buffer_cap)
    {
      _native_write_buffer = std::make_unique<char[]>(unwritten_size);
      _native_write_buffer_cap = unwritten_size;
    }

    if (bytes_written < first_size)
    {
      std::memcpy(_native_write_buffer.get(), first + bytes_written, first_size - bytes_written);

      if (second_size != 0)
      {
        std::memcpy(_native_write_buffer.get() + first_size - bytes_written, second, second_size);
      }
    }
    else
    {
      std::memcpy(_native_write_buffer.get(), second + (bytes_written - first_size), unwritten_size);
    }

    _native_write_pos = unwritten_size;
  }

  // The functions from here to the member variables are what differs between Windows and POSIX

#if defined(_WIN32)
private:
  friend class detail::SinkManager;

  static std::string _file_sink_id(fs::path const& normalized_path, bool& resolved)
  {
    // The id does not depend on whether the file exists
    resolved = false;

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

protected:
  using OpenedFileGuard = detail::OpenedHandleGuard;

  /** How the errors of a failed write and of a failed sync start. _last_error_message() follows */
  static constexpr char const* _write_failed_message{"WriteFile failed. "};
  static constexpr char const* _sync_failed_message{"FlushFileBuffers failed. "};

  /** The reason of the platform call that has just failed, as a number that is kept without memory */
  QUILL_NODISCARD static DWORD _last_error() noexcept { return ::GetLastError(); }

  QUILL_NODISCARD static std::string _error_message(DWORD error)
  {
    return "GetLastError: " + std::to_string(error);
  }

  /** The reason of the platform call that has just failed */
  QUILL_NODISCARD static std::string _last_error_message() { return _error_message(_last_error()); }

  /** Windows has no /dev/null device: the sink of that name opens no file and discards its output */
  QUILL_NODISCARD bool _opens_file() const noexcept { return !is_null(); }

  QUILL_NODISCARD bool is_open() const noexcept
  {
    return _native_file_handle != INVALID_HANDLE_VALUE;
  }

  /** The open file, as the callbacks of the FileEventNotifier receive it */
  QUILL_NODISCARD HANDLE& _opened_file() noexcept { return _native_file_handle; }

  /** @return false if the file was not opened, which is only returned without exceptions */
  QUILL_NODISCARD bool _open_native_file(OpenedFileGuard& opened_file_guard,
                                         fs::path const& filename, std::string const& mode)
  {
    // Retry only while another process holds the file, e.g. an antivirus scan. Other errors such
    // as a missing directory are not transient and must not stall the backend thread.
    constexpr int max_attempts{3};
    constexpr uint64_t retry_delay_ns{200ull * 1'000'000ull};
    DWORD const creation_disposition = (!mode.empty() && (mode[0] == 'w')) ? CREATE_ALWAYS : OPEN_ALWAYS;

    for (int attempt{1};; ++attempt)
    {
      opened_file_guard.handle = ::CreateFileW(
        filename.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, creation_disposition, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);

      if (opened_file_guard.handle != INVALID_HANDLE_VALUE)
      {
        break;
      }

      DWORD const last_error = ::GetLastError();
      bool const is_transient = (last_error == ERROR_SHARING_VIOLATION) || (last_error == ERROR_LOCK_VIOLATION);

      if (!is_transient || (attempt == max_attempts))
      {
        QUILL_THROW_OR_RETURN_VALUE(
          QuillError{std::string{"CreateFileW failed after "} + std::to_string(attempt) +
                     " attempt(s), path: " + detail::file_sink_path_string(filename) +
                     " mode: " + mode + " GetLastError: " + std::to_string(last_error)},
          false);
      }

      detail::sleep_for_ns(retry_delay_ns);
    }

    // The writes to the file that is being opened follow it
    _append_mode = !mode.empty() && mode[0] == 'a';
    return true;
  }

  /**
   * Calls FileEventNotifier::after_open with the file as it was opened, before the sink writes
   * to it
   * @return false if it failed, which is only returned without exceptions
   */
  QUILL_NODISCARD bool _notify_after_open(HANDLE file_handle, fs::path const& filename)
  {
    if (_append_mode && ::GetFileType(file_handle) == FILE_TYPE_DISK)
    {
      LARGE_INTEGER offset{};
      if (!::SetFilePointerEx(file_handle, offset, nullptr, FILE_END))
      {
        DWORD const seek_error = ::GetLastError();
        QUILL_THROW_OR_RETURN_VALUE(
          QuillError{"SetFilePointerEx failed. GetLastError: " + std::to_string(seek_error)}, false);
      }
    }

    _file_event_notifier.after_open(filename, file_handle);
    return true;
  }

  /**
   * @return false if the file was closed without all that was written to it, which is only
   * returned without exceptions
   */
  bool close_file()
  {
    if (_native_file_handle == INVALID_HANDLE_VALUE)
    {
      return true;
    }

    bool flushed{true};

    if (_file_event_notifier.before_close)
    {
      // Callbacks write through the handle, outside Quill's separate native buffer. Without
      // exceptions a flush that fails returns: what it did not write is dropped, so that it does
      // not follow what the callback writes
      flushed = _flush_native_write_buffer();
      _native_write_pos = 0;
      QUILL_TRY { _file_event_notifier.before_close(_filename, _native_file_handle); }
  #if !defined(QUILL_NO_EXCEPTIONS)
      QUILL_CATCH_ALL()
      {
        HANDLE native_file_handle = _native_file_handle;
        QUILL_TRY { (void)_flush_native_write_buffer(); }
        QUILL_CATCH_ALL() {}
        _native_file_handle = INVALID_HANDLE_VALUE;
        _append_mode = false;
        _native_write_pos = 0;
        ::CloseHandle(native_file_handle);
        throw;
      }
  #endif
    }

    // Without exceptions a flush that fails returns: the file is closed without those records
    flushed = _flush_native_write_buffer() && flushed;
    ::CloseHandle(_native_file_handle);
    _native_file_handle = INVALID_HANDLE_VALUE;
    _append_mode = false;
    _native_write_pos = 0;

    if (_file_event_notifier.after_close)
    {
      _file_event_notifier.after_close(_filename);
    }

    return flushed;
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

  /** @return false if the file was not synced, with the reason in GetLastError() */
  QUILL_NODISCARD bool _sync_native_file() noexcept
  {
    return ::FlushFileBuffers(_native_file_handle) != 0;
  }

  /** @return false if the file was not emptied, with the reason in GetLastError() */
  QUILL_NODISCARD static bool _truncate_native_file(HANDLE file_handle) noexcept
  {
    LARGE_INTEGER const start{};
    return (::SetFilePointerEx(file_handle, start, nullptr, FILE_BEGIN) != 0) &&
      (::SetEndOfFile(file_handle) != 0);
  }

  /** The file has no stream that buffers: the write buffer had every record that was written */
  QUILL_NODISCARD bool _flush_stream() noexcept
  {
    _write_occurred = false;
    return true;
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

  /** @return true if the open file is no longer at its path */
  QUILL_NODISCARD bool _is_file_removed() const
  {
    std::error_code ec;
    bool const file_exists = fs::exists(_filename, ec);

    if (ec)
    {
      // libc++ can report access denied for a file pending deletion. Query the open
      // handle so an unrelated status error does not cause a valid file to be reopened.
      FILE_STANDARD_INFO file_info{};
      return ::GetFileInformationByHandleEx(_native_file_handle, FileStandardInfo, &file_info, sizeof(file_info)) &&
        file_info.DeletePending;
    }

    return !file_exists;
  }

  /**
   * Writes a record past the write buffer: one that is larger than it, and every record when the
   * buffer is disabled. The record can come in two parts, the frame of a binary record and its
   * bytes. When a write fails, what was written of the record stays in the file, and the buffer
   * keeps the rest for the next flush, also when it is disabled.
   */
  void _write_to_file(char const* first, size_t first_size, char const* second, size_t second_size)
  {
    size_t bytes_written = _write_to_handle(_native_file_handle, _append_mode, first, first_size);

    if ((bytes_written == first_size) && (second_size != 0))
    {
      bytes_written += _write_to_handle(_native_file_handle, _append_mode, second, second_size);
    }

    if (QUILL_UNLIKELY(bytes_written != (first_size + second_size)))
    {
      DWORD const last_error = ::GetLastError();
      _keep_unwritten_part(first, first_size, second, second_size, bytes_written);

      QUILL_THROW_OR_RETURN(
        QuillError{std::string{"WriteFile failed. GetLastError: "} + std::to_string(last_error)});
    }
  }

  /** Writes to a file that is open or that open_file() is opening, see _write_to_handle() */
  QUILL_NODISCARD size_t _write_to_native_file(HANDLE file_handle, char const* data, size_t size) const noexcept
  {
    return _write_to_handle(file_handle, _append_mode, data, size);
  }

  /**
   * Writes to the handle until all of `data` is in the file. Each write tells how many bytes are
   * in the file, so a partial write is continued.
   * @return the number of bytes that are in the file: fewer than `size` when a write failed, with
   * the reason in GetLastError()
   */
  QUILL_NODISCARD static size_t _write_to_handle(HANDLE file_handle, bool append_mode,
                                                 char const* data, size_t size) noexcept
  {
    size_t total_bytes_written{0};

    while (total_bytes_written != size)
    {
      DWORD bytes_written = 0;
      constexpr size_t max_dword = static_cast<size_t>(~DWORD{0});
      DWORD const chunk = static_cast<DWORD>(std::min<size_t>(size - total_bytes_written, max_dword));
      OVERLAPPED overlapped{};
      OVERLAPPED* overlapped_ptr{nullptr};

      if (append_mode)
      {
        overlapped.Offset = 0xFFFFFFFF;
        overlapped.OffsetHigh = 0xFFFFFFFF;
        overlapped_ptr = &overlapped;
      }

      if (!::WriteFile(file_handle, data + total_bytes_written, chunk, &bytes_written, overlapped_ptr) ||
          (bytes_written == 0))
      {
        break;
      }

      total_bytes_written += bytes_written;
    }

    return total_bytes_written;
  }
#else
private:
  struct OpenedFileGuard
  {
    ~OpenedFileGuard()
    {
      if (handle)
      {
        std::fclose(handle);
      }
    }

    QUILL_NODISCARD FILE* release() noexcept
    {
      FILE* const released_handle = handle;
      handle = nullptr;
      return released_handle;
    }

    FILE* handle{nullptr};
  };

protected:
  /** How the errors of a failed write and of a failed sync start. _last_error_message() follows */
  static constexpr char const* _write_failed_message{"write failed "};
  static constexpr char const* _sync_failed_message{"fsync failed "};

  /** The reason of the platform call that has just failed, as a number that is kept without memory */
  QUILL_NODISCARD static int _last_error() noexcept { return errno; }

  QUILL_NODISCARD static std::string _error_message(int error)
  {
    return "errno: " + std::to_string(error) + " error: " + std::strerror(error);
  }

  /** The reason of the platform call that has just failed */
  QUILL_NODISCARD static std::string _last_error_message() { return _error_message(_last_error()); }

  /** The sink of /dev/null opens the device, as the sink of a file does */
  QUILL_NODISCARD bool _opens_file() const noexcept { return true; }

  QUILL_NODISCARD bool is_open() const noexcept { return _file != nullptr; }

  /** The open file, as the callbacks of the FileEventNotifier receive it */
  QUILL_NODISCARD FILE*& _opened_file() noexcept { return _file; }

  /** @return false if the file was not opened, which is only returned without exceptions */
  QUILL_NODISCARD bool _open_native_file(OpenedFileGuard& opened_file_guard,
                                         fs::path const& filename, std::string const& mode)
  {
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
      opened_file_guard.handle = ::fdopen(fd, mode.data());
      if (!opened_file_guard.handle)
      {
        int const fdopen_errno = errno;
        ::close(fd);
        errno = fdopen_errno;
      }
      else if ((_config.write_buffer_size() == 0) &&
               (_config.binary_record_framing() != BinaryRecordFraming::None))
      {
        // Unbuffered, the stream takes only what reaches the file, so that the rest of a binary
        // record that a write cuts short is kept for the next flush, see _write_to_stream()
        std::setvbuf(opened_file_guard.handle, nullptr, _IONBF, 0);
      }
    }

    if (!opened_file_guard.handle)
    {
      // Building the message can change errno
      int const open_errno = errno;
      QUILL_THROW_OR_RETURN_VALUE(
        QuillError{std::string{"fopen failed, path: "} + filename.string() + " mode: " + mode +
                   " errno: " + std::to_string(open_errno) + " error: " + std::strerror(open_errno)},
        false);
    }

    return true;
  }

  /**
   * Calls FileEventNotifier::after_open with the file as it was opened, before the sink writes
   * to it
   * @return false if it failed, which is only returned without exceptions
   */
  QUILL_NODISCARD bool _notify_after_open(FILE* file, fs::path const& filename)
  {
    _file_event_notifier.after_open(filename, file);

    // What the callback wrote to the stream is part of the size of the file
    return _flush_opened_file(file, filename);
  }

  /**
   * @return false if the file was closed without all that was written to it, which is only
   * returned without exceptions
   */
  bool close_file()
  {
    if (!_file)
    {
      return true;
    }

    bool flushed{true};

    if (_file_event_notifier.before_close)
    {
      // Callbacks write through the stream, outside Quill's separate native buffer. Without
      // exceptions a flush that fails returns: what it did not write is dropped, so that it does
      // not follow what the callback writes
      flushed = _flush_native_write_buffer();
      _native_write_pos = 0;
      QUILL_TRY { _file_event_notifier.before_close(_filename, _file); }
  #if !defined(QUILL_NO_EXCEPTIONS)
      QUILL_CATCH_ALL()
      {
        FILE* file = _file;
        _file = nullptr;
        _native_write_pos = 0;
        std::fclose(file);
        throw;
      }
  #endif
    }

    // Without exceptions a flush that fails returns: the file is closed without those records
    flushed = _flush_native_write_buffer() && flushed;
    FILE* file = _file;
    _file = nullptr;
    _native_write_pos = 0;

    if (std::fclose(file) != 0)
    {
      int const saved_errno = errno;
      QUILL_THROW_OR_RETURN_VALUE(QuillError{std::string{"fclose failed errno: "} + std::to_string(saved_errno) +
                                             " error: " + std::strerror(saved_errno)},
                                  false);
    }

    if (_file_event_notifier.after_close)
    {
      _file_event_notifier.after_close(_filename);
    }

    return flushed;
  }

  void _close_file_noexcept() noexcept
  {
    QUILL_TRY { close_file(); }
  #if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH_ALL()
    {
      if (_file)
      {
        FILE* file = _file;
        _file = nullptr;
        std::fclose(file);
      }
    }
  #endif
  }

  /** @return false if the file was not synced, with the reason in errno */
  QUILL_NODISCARD bool _sync_native_file() noexcept
  {
    // Retry on EINTR — a signal delivered during fsync() causes it to fail transiently.
    int ret{0};
    do
    {
      ret = ::fsync(fileno(_file));
    } while (ret != 0 && errno == EINTR);

    return ret == 0;
  }

  /** @return false if the file was not emptied, with the reason in errno */
  QUILL_NODISCARD static bool _truncate_native_file(FILE* file) noexcept
  {
    return ::ftruncate(fileno(file), 0) == 0;
  }

  /**
   * The stream has the records when the write buffer is disabled. Clears _write_occurred
   * @return false if the flush failed, which is only returned without exceptions
   */
  QUILL_NODISCARD bool _flush_stream() { return flush(); }

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
   * @return false if the flush failed, which is only returned without exceptions
   */
  QUILL_NODISCARD static bool _flush_opened_file(FILE* file, fs::path const& filename)
  {
    if (std::fflush(file) != 0)
    {
      int const flush_errno = errno;
      QUILL_THROW_OR_RETURN_VALUE(
        QuillError{std::string{"fflush failed, path: "} + filename.string() +
                   " errno: " + std::to_string(flush_errno) + " error: " + std::strerror(flush_errno)},
        false);
    }

    return true;
  }

  /** @return true if the open file is no longer at its path */
  QUILL_NODISCARD bool _is_file_removed() const
  {
    std::error_code ec;
    bool const file_exists = fs::exists(_filename, ec);
    return !ec && !file_exists;
  }

  /**
   * Writes a record past the write buffer: one that is larger than it, and every record when the
   * buffer is disabled. The record can come in two parts, the frame of a binary record and its
   * bytes. When a write fails, what was written of the record stays in the file, and the buffer
   * keeps the rest for the next flush, also when it is disabled.
   */
  void _write_to_file(char const* first, size_t first_size, char const* second, size_t second_size)
  {
    if (_native_write_buffer_cap == 0)
    {
      _write_to_stream(first, first_size, second, second_size);
      return;
    }

    int const file_descriptor = fileno(_file);
    size_t bytes_written = _write_to_descriptor(file_descriptor, first, first_size);

    if ((bytes_written == first_size) && (second_size != 0))
    {
      bytes_written += _write_to_descriptor(file_descriptor, second, second_size);
    }

    if (QUILL_UNLIKELY(bytes_written != (first_size + second_size)))
    {
      int const write_errno = errno;
      _keep_unwritten_part(first, first_size, second, second_size, bytes_written);
      QUILL_THROW_OR_RETURN(QuillError{std::string{"write failed errno: "} + std::to_string(write_errno) +
                                       " error: " + std::strerror(write_errno)});
    }
  }

  /**
   * Writes a record through the stream, as the callbacks of the FileEventNotifier have configured
   * it, when the buffer is disabled. When a write fails, the buffer keeps what the stream did not
   * take, as for a write to the file. An unbuffered stream has written what it took; a buffered
   * one can have discarded a part of it, which is then missing from the file
   */
  void _write_to_stream(char const* first, size_t first_size, char const* second, size_t second_size)
  {
    size_t first_written{0};
    size_t second_written{0};

    QUILL_TRY
    {
      // The second part is not written without the first: it would be a record without its frame
      if (_safe_fwrite(first, sizeof(char), first_size, _file, false, first_written) &&
          ((second_size == 0) || _safe_fwrite(second, sizeof(char), second_size, _file, false, second_written)))
      {
        return;
      }
    }
  #if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH_ALL()
    {
      _keep_unwritten_part(first, first_size, second, second_size, first_written + second_written);
      throw;
    }
  #endif

    // Without exceptions a write that fails returns
    _keep_unwritten_part(first, first_size, second, second_size, first_written + second_written);
  }

  /** Writes to a file that is open or that open_file() is opening, see _write_to_descriptor() */
  QUILL_NODISCARD static size_t _write_to_native_file(FILE* file, char const* data, size_t size) noexcept
  {
    return _write_to_descriptor(fileno(file), data, size);
  }

  /**
   * Writes to the file descriptor until all of `data` is in the file. Each write tells how many
   * bytes are in the file, so an interrupted or partial write is continued.
   * @return the number of bytes that are in the file: fewer than `size` when a write failed, with
   * the reason in errno
   */
  QUILL_NODISCARD static size_t _write_to_descriptor(int file_descriptor, char const* data, size_t size) noexcept
  {
    size_t total_bytes_written{0};

    while (total_bytes_written != size)
    {
      ssize_t const bytes_written =
        ::write(file_descriptor, data + total_bytes_written, size - total_bytes_written);

      if (bytes_written > 0)
      {
        total_bytes_written += static_cast<size_t>(bytes_written);
        continue;
      }

      if (bytes_written == 0)
      {
        // Reported as an error rather than tried without end
        errno = EIO;
        break;
      }

      // Nothing was written by an interrupted call, so it is made again
      if (errno != EINTR)
      {
        break;
      }
    }

    return total_bytes_written;
  }
#endif

protected:
  FileSinkConfig _config;
  std::chrono::steady_clock::time_point _last_fsync_timestamp{};
  uint64_t _initial_file_size{0}; /**< Size of the open file when open_file() had created it, 0 if it had content */
  bool _fsync_pending{false}; /**< An fsync was skipped because of minimum_fsync_interval, or a header is not synced */
  bool _flush_failed{false}; /**< The last flush_sink() returned without the records written and synced, which it only does without exceptions */
  bool _sync_failed{false}; /**< The last fsync of the file failed, also one of the last flush_sink() */
#if defined(_WIN32)
  HANDLE _native_file_handle{INVALID_HANDLE_VALUE};
#endif
  std::unique_ptr<char[]> _native_write_buffer;
  size_t _native_write_pos{0};
  size_t _native_write_buffer_cap{0};
#if defined(_WIN32)
  bool _append_mode{false};
#endif
};

QUILL_END_EXPORT

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
  #pragma warning(pop)
#endif

QUILL_END_NAMESPACE
