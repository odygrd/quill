/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Filesystem.h"
#include "quill/core/LogLevel.h"
#include "quill/core/QuillError.h"
#include "quill/sinks/Sink.h"

#ifndef QUILL_MODULE
  #include <cerrno>
  #include <cstdint>
  #include <cstdio>
  #include <cstring>
  #include <functional>
  #include <optional>
  #include <string>
  #include <string_view>
  #include <system_error>
  #include <utility>
  #include <vector>
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
  #include <windows.h>
#endif

QUILL_BEGIN_NAMESPACE

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
  #pragma warning(push)
  #pragma warning(disable : 4996)
#endif

QUILL_BEGIN_EXPORT

#if defined(_WIN32)
using FileEventNotifierHandle = HANDLE;
#else
using FileEventNotifierHandle = FILE*;
#endif

/**
 * @brief Notifies on file events by calling the appropriate callback.
 *
 * @note `before_write` executes as part of the normal log write path.
 *       `before_open`, `after_open`, `before_close`, and `after_close` execute on the thread
 *       performing the file open/close operation. Different callbacks, and different invocations
 *       of the same callback, may therefore run on different threads over the sink lifetime.
 *       Callbacks must be thread-safe and must not assume a single calling thread.
 *       On Windows, append-mode regular files are positioned at the end before `after_open`.
 *
 * @note `after_open` is called with the file as it was opened, before the sink writes to it, e.g.
 *       the header of FileSinkConfig::set_file_header(). An exception from the callback leaves
 *       the sink without an open file, so it can reject a file that the sink must not append to.
 *
 * @note FileSink keeps the records in a write buffer of its own and writes it to the file past
 *       the handle of `after_open` and `before_close`. What a callback writes through the handle
 *       stays in order with the records: the buffer is written to the file before `before_close`.
 *       The records do not follow how a callback configures the handle, e.g. with `setvbuf`, and
 *       the handle is not to be used outside the callback, unless
 *       FileSinkConfig::set_write_buffer_size() disables the buffer with a size of 0, which makes
 *       the sink write every record through the handle.
 *
 * @note `before_write` transforms everything that the sink writes: each log statement, each
 *       binary record and the header of a file. The frame of a binary record, see
 *       FileSinkConfig::set_binary_record_framing(), has the size of the transformed bytes.
 *
 * @warning File-event callbacks must not call `Frontend::create_sink()` or
 *          `Frontend::create_or_get_sink()` with the name of the sink currently being constructed.
 *          That sink name remains reserved until construction completes, so recursively requesting
 *          it waits indefinitely. Creating a sink with a different name is supported.
 *
 * @note When `before_write` changes the message size, size-based rotation
 *       (`rotation_max_file_size`) evaluates its threshold using the pre-transform size of the
 *       current message, so a rotation can trigger up to one transformed message later or
 *       earlier than the exact limit. The tracked file size itself stays accurate.
 */
struct FileEventNotifier
{
  std::function<void(fs::path const& file_path)> before_open;
  std::function<void(fs::path const& file_path, FileEventNotifierHandle f)> after_open;
  std::function<void(fs::path const& file_path, FileEventNotifierHandle f)> before_close;
  std::function<void(fs::path const& file_path)> after_close;
  std::function<std::string(std::string_view message)> before_write;
};

/**
 * @brief StreamSink class for handling log messages
 */
class StreamSink : public Sink
{
public:
  /**
   * @brief Constructor for StreamSink
   * @param stream The stream type (stdout, stderr, or file)
   * @param override_pattern_formatter_options override the logger pattern formatter
   * @param file Already-open FILE* owned by the caller; this sink does not open or close it
   * @param file_event_notifier Notifies on file events
   * @param stream_is_unbuffered Opt-in EINTR retries for a caller-owned, unbuffered stream.
   *        Leave false for buffered streams. This flag does not change the buffering mode.
   *
   * @note For this opt-in path, open the FILE* yourself with fopen() or POSIX fdopen(), then
   *       successfully call `setvbuf(file, nullptr, _IONBF, 0)` before any I/O and before
   *       constructing the sink. Pass that FILE* as `file` and true as `stream_is_unbuffered`.
   *       Keep buffering disabled and the FILE* alive until all backend use has finished.
   *       See docs/snippets/quill_docs_example_unbuffered_stream.cpp for a complete example.
   *
   * @note This option applies to direct StreamSink construction. FileSink and ConsoleSink
   *       leave it false; configuring a FileSink stream in FileEventNotifier::after_open
   *       does not enable it. Buffered short writes still report errors; this option does
   *       not recover data discarded by an interrupted stdio buffer flush.
   * @throws QuillError if an invalid parameter is provided
   */
  explicit StreamSink(fs::path stream, FILE* file = nullptr,
                      std::optional<PatternFormatterOptions> const& override_pattern_formatter_options = std::nullopt,
                      FileEventNotifier file_event_notifier = FileEventNotifier{},
                      bool stream_is_unbuffered = false)
    : Sink(override_pattern_formatter_options),
      _filename(std::move(stream)),
      _file(file),
      _file_event_notifier(std::move(file_event_notifier)),
      _stream_is_unbuffered(stream_is_unbuffered)
  {
    // reserve stdout and stderr as filenames
    if (_filename == std::string{"stdout"})
    {
      _file = stdout;
    }
    else if (_filename == std::string{"stderr"})
    {
      _file = stderr;
    }
    else if (_filename == std::string{"/dev/null"})
    {
      _is_null = true;
    }
    else
    {
      _filename = detail::normalize_file_sink_path(_filename);
    }
  }

  ~StreamSink() override = default;

  /**
   * @brief Writes a formatted log message to the stream
   */
  QUILL_ATTRIBUTE_HOT void write_log(LogRecord const& record) override
  {
    _write_bytes(record.log_statement);
  }

  /**
   * @brief Writes the bytes of a binary record to the stream as they are. Nothing is added before
   * or after them. FileEventNotifier::before_write transforms them first, as it does a log
   * statement.
   * @note A caller-owned FILE* must be opened in binary mode on Windows.
   */
  QUILL_ATTRIBUTE_HOT void write_binary(BinaryRecord const& record) override
  {
    _write_bytes(std::string_view{reinterpret_cast<char const*>(record.data), record.size});
  }

  /**
   * Flushes the stream
   */
  QUILL_ATTRIBUTE_HOT void flush_sink(SinkFlushReason /* flush_reason */) override
  {
    if (!_write_occurred || !_file)
    {
      return;
    }

    flush();
  }

  /**
   * @brief Returns the name of the file
   * @return The name of the file
   */
  QUILL_NODISCARD virtual fs::path const& get_filename() const noexcept { return _filename; }

  /**
   * @brief Checks if the stream is null
   * @return True if the stream is null, false otherwise
   */
  QUILL_NODISCARD bool is_null() const noexcept { return _is_null; }

  /**
   * @brief Writes data safely to the stream
   * @param ptr Pointer to the data to be written
   * @param size Size of each element to be written
   * @param count Number of elements to write
   * @param stream The stream to write to
   */
  QUILL_ATTRIBUTE_HOT static void safe_fwrite(void const* ptr, size_t size, size_t count, FILE* stream)
  {
    _safe_fwrite(ptr, size, count, stream, false);
  }

  /**
   * @brief Writes data to an unbuffered stream, retrying interrupted writes.
   * @param ptr Pointer to the data to be written
   * @param size Size of each element to be written
   * @param count Number of elements to write
   * @param stream Stream configured with `_IONBF` before any I/O, with buffering still disabled
   * @note The caller opens and owns the FILE*. Configure it immediately after fopen() or
   *       POSIX fdopen(); see the StreamSink constructor documentation for the setup sequence.
   */
  QUILL_ATTRIBUTE_HOT static void safe_fwrite_unbuffered(void const* ptr, size_t size, size_t count, FILE* stream)
  {
    _safe_fwrite(ptr, size, count, stream, true);
  }

protected:
  QUILL_ATTRIBUTE_HOT static void _safe_fwrite(void const* ptr, size_t size, size_t count,
                                               FILE* stream, bool stream_is_unbuffered)
  {
    size_t const total_bytes = size * count;
    size_t bytes_written = 0;

    while (bytes_written < total_bytes)
    {
      auto const* current_ptr = static_cast<char const*>(ptr) + bytes_written;
      size_t const remaining = total_bytes - bytes_written;

#if defined(_WIN32)
      // On Windows, using fwrite to non-binary stream (stdout/stderr) results in \r\r\n
      // Instead, use WriteFile for console streams
      if ((stream == stdout) || (stream == stderr))
      {
        HANDLE const handle = reinterpret_cast<HANDLE>(::_get_osfhandle(::_fileno(stream)));

        if (QUILL_LIKELY(handle != INVALID_HANDLE_VALUE))
        {
          constexpr size_t max_dword = static_cast<size_t>(~DWORD{0});
          auto const total_bytes_remaining = static_cast<DWORD>(std::min<size_t>(remaining, max_dword));
          DWORD bytes_written_this_call = 0;

          if (QUILL_UNLIKELY(::WriteFile(handle, current_ptr, total_bytes_remaining,
                                         &bytes_written_this_call, nullptr) == 0))
          {
            QUILL_THROW(QuillError{std::string{"WriteFile failed. GetLastError: "} +
                                   std::to_string(::GetLastError())});
          }

          if (QUILL_UNLIKELY(bytes_written_this_call == 0))
          {
            QUILL_THROW(QuillError{"WriteFile returned 0 bytes written without error"});
          }

          bytes_written += bytes_written_this_call;
          continue;
        }

        // Fall through to fwrite if the handle is invalid (e.g., no console attached)
      }
#endif

      size_t const written = std::fwrite(current_ptr, 1, remaining, stream);

      if (QUILL_UNLIKELY(written < remaining))
      {
        // Partial write or error
        if (ferror(stream))
        {
          int const saved_errno = errno;
          std::clearerr(stream); // Reset error state
          // Buffered fwrite can count bytes that were discarded by a failed flush.
          if (saved_errno == EINTR && stream_is_unbuffered)
          {
            bytes_written += written;
            continue;
          }
          QUILL_THROW(QuillError{std::string{"fwrite failed errno: "} + std::to_string(saved_errno) +
                                 " error: " + std::strerror(saved_errno)});
        }

        if (written == 0)
        {
          // Zero bytes written without error is unusual - treat as fatal to avoid infinite loop
          QUILL_THROW(
            QuillError{std::string{"fwrite returned 0 bytes written without error - stream may be "
                                   "at EOF or in invalid state"}});
        }

        // Partial write succeeded - continue with remaining bytes
      }

      bytes_written += written;
    }
  }

  QUILL_NODISCARD virtual size_t estimate_write_size(LogRecord const& record)
  {
    return record.log_statement.size();
  }

  /**
   * Discards any state estimate_write_size() may have cached for the write that follows it.
   * Called when that write is abandoned (e.g. an exception during file rotation) so a later,
   * unrelated record cannot consume the stale cached state.
   */
  void discard_write_estimate() noexcept {}

  /**
   * Writes a log statement or the bytes of a binary record, after before_write has transformed
   * them.
   */
  QUILL_ATTRIBUTE_HOT void _write_bytes(std::string_view bytes)
  {
    if (QUILL_UNLIKELY(!_file))
    {
      // FileSink::flush() tries to re-open a deleted file and if it fails _file can be null
      return;
    }

    if (_file_event_notifier.before_write)
    {
      std::string const user_bytes = _file_event_notifier.before_write(bytes);
      _write_statement(user_bytes);
    }
    else
    {
      _write_statement(bytes);
    }
  }

  /**
   * Writes an already transformed statement without invoking before_write again.
   */
  QUILL_ATTRIBUTE_HOT void _write_statement(std::string_view statement)
  {
    _safe_fwrite(statement.data(), sizeof(char), statement.size(), _file, _stream_is_unbuffered);
    _file_size += statement.size();
    _write_occurred = true;
  }

  /**
   * Flushes the stream
   */
  QUILL_ATTRIBUTE_HOT void flush()
  {
    // Retry on EINTR — a signal delivered during fflush() causes it to fail transiently.
    int result{0};
    do
    {
      result = std::fflush(_file);
    } while (result != 0 && errno == EINTR);

    if (QUILL_LIKELY(result == 0))
    {
      _write_occurred = false;
    }
    else
    {
      int const saved_errno = errno;
      std::clearerr(_file); // Reset error state
      QUILL_THROW(QuillError{std::string{"fflush failed errno: "} + std::to_string(saved_errno) +
                             " error: " + std::strerror(saved_errno)});
    }
  }

protected:
  fs::path _filename;
  FILE* _file{nullptr};
  uint64_t _file_size{0}; /**< The current file size */
  FileEventNotifier _file_event_notifier;
  bool _is_null{false};
  bool _write_occurred{false};
  bool const _stream_is_unbuffered{false};
};

QUILL_END_EXPORT

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
  #pragma warning(pop)
#endif

QUILL_END_NAMESPACE
