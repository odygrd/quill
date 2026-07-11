/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Frontend.h"
#include "quill/core/Attributes.h"
#include "quill/core/QuillError.h"
#include "quill/sinks/FileSink.h"
#include "quill/sinks/RotatingFileSink.h"
#include "quill/sinks/Sink.h"
#include "quill/sinks/StreamSink.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <utility>

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

/**
 * @brief A CSV writer class for asynchronous logging of CSV files.
 *
 * This class facilitates the asynchronous logging of CSV files, where formatting
 * and I/O operations are handled by the backend worker thread.
 *
 * Call close() before Backend::stop() when deterministic logger removal and file closure
 * are required. The destructor performs best-effort asynchronous cleanup and does not block.
 *
 * @tparam TCsvSchema A user-defined struct specifying the CSV schema at compile-time.
 * @tparam TFrontendOptions Custom frontend options if they are used application-wide. If no custom frontend options are used, then use quill::FrontendOptions.
 *
 * The TCsvSchema struct should define the CSV header and format, for example:
 *
 * @code
 * struct OrderCsvSchema
 * {
 *   static constexpr char const* header = "order_id,symbol,quantity,price,side";
 *   static constexpr char const* format = "{},{},{},{:.2f},{}";
 * };
 * @endcode
 */
template <typename TCsvSchema, typename TFrontendOptions>
class CsvWriter
{
public:
  using frontend_t = FrontendImpl<TFrontendOptions>;

  CsvWriter(CsvWriter const&) = delete;
  CsvWriter& operator=(CsvWriter const&) = delete;
  CsvWriter(CsvWriter&&) = delete;
  CsvWriter& operator=(CsvWriter&&) = delete;

  /**
   * Constructs a CsvWriter object that writes to a file.
   *
   * @param filename The name of the CSV file to write to.
   * @param open_mode The mode in which to open the file ('w' for write, 'a' for append).
   * @param filename_append Option to append to the filename (None, StartDate, StartDateTime).
   */
  explicit CsvWriter(std::string const& filename, char open_mode = 'w',
                     FilenameAppendOption filename_append = FilenameAppendOption::None)
  {
    auto sink = frontend_t::template create_or_get_sink<FileSink>(filename,
                                                                  [open_mode, filename_append]()
                                                                  {
                                                                    FileSinkConfig cfg;
                                                                    cfg.set_open_mode(open_mode);
                                                                    cfg.set_filename_append_option(filename_append);
                                                                    return cfg;
                                                                  }());

    bool const should_write_header = _should_write_header_on_initial_file(sink, open_mode == 'a', true);

    _logger = frontend_t::create_or_get_logger(_make_logger_name(filename), std::move(sink),
                                               PatternFormatterOptions{"%(message)", "", Timezone::GmtTime});

    if (should_write_header)
    {
      write_header();
    }
  }

  /**
   * Constructs a CsvWriter object that writes to a file.
   *
   * @param filename The name of the CSV file to write to.
   * @param sink_config Configuration settings for the file sink.
   * @param should_write_header Whether to write the header at the beginning of the CSV file.
   */
  CsvWriter(std::string const& filename, FileSinkConfig sink_config, bool should_write_header = true)
  {
    auto sink = frontend_t::template create_or_get_sink<FileSink>(filename, sink_config);
    bool const should_write_initial_header = _should_write_header_on_initial_file(
      sink, _is_append_mode(sink_config.open_mode()), should_write_header);

    _logger = frontend_t::create_or_get_logger(_make_logger_name(filename), std::move(sink),
                                               PatternFormatterOptions{"%(message)", "", Timezone::GmtTime});

    if (should_write_initial_header)
    {
      write_header();
    }
  }

  /**
   * Constructs a CsvWriter object that writes to a rotating file.
   *
   * @param filename The name of the CSV file to write to.
   * @param sink_config Configuration settings for the file sink.
   * @param should_write_header Whether to write the header at the beginning of the CSV file.
   */
  CsvWriter(std::string const& filename, RotatingFileSinkConfig sink_config, bool should_write_header = true)
  {
    FileEventNotifier file_notifier;
    bool const should_write_initial_header = should_write_header;

    // The sink owns this notifier and may invoke it after the CsvWriter is destroyed.
    // Keep it independent from this instance and only use schema-level state.
    file_notifier.after_open = [should_write_header, is_first_rotation = true](
                                 fs::path const&, FileEventNotifierHandle file) mutable
    {
      if (is_first_rotation)
      {
        // On the first rotation, skip writing the header because the logger isn't fully initialized.
        is_first_rotation = false;
        return;
      }

      if (should_write_header)
      {
        // For subsequent rotations, if header writing is enabled, append the header directly
        _write_header(file);
      }
    };

    auto sink = frontend_t::template create_or_get_sink<RotatingFileSink>(filename, sink_config, file_notifier);
    bool const should_write_header_for_initial_file = _should_write_header_on_initial_file(
      sink, _is_append_mode(sink_config.open_mode()), should_write_initial_header);

    _logger = frontend_t::create_or_get_logger(_make_logger_name(filename), std::move(sink),
                                               PatternFormatterOptions{"%(message)", "", Timezone::GmtTime});

    // For the initial file (before any rotations), write the header if required.
    if (should_write_header_for_initial_file)
    {
      write_header();
    }
  }

  /**
   * Constructs a CsvWriter object that writes to a specified sink.
   *
   * @param unique_name A unique name for this CsvWriter instance.
   * @param sink The sink to output the data to (e.g., a ConsoleSink or a user-defined Sink).
   * @param should_write_header Whether to write the header at the beginning of the CSV file.
   */
  CsvWriter(std::string const& unique_name, std::shared_ptr<Sink> sink, bool should_write_header = true)
  {
    _logger = frontend_t::create_or_get_logger(_make_logger_name(unique_name), std::move(sink),
                                               PatternFormatterOptions{"%(message)", "", Timezone::GmtTime});

    if (should_write_header)
    {
      write_header();
    }
  }

  /**
   * Constructs a CsvWriter object that writes to multiple sinks.
   *
   * @param unique_name A unique name for this CsvWriter instance.
   * @param sinks A list of sinks to output the data to.
   * @param should_write_header Whether to write the header at the beginning of the CSV file.
   */
  CsvWriter(std::string const& unique_name, std::vector<std::shared_ptr<Sink>> sinks,
            bool should_write_header = true)
  {
    _logger = frontend_t::create_or_get_logger(
      _make_logger_name(unique_name), sinks, PatternFormatterOptions{"%(message)", "", Timezone::GmtTime});

    if (should_write_header)
    {
      write_header();
    }
  }

  /**
   * Destructor for CsvWriter.
   *
   * The destructor performs best-effort asynchronous cleanup. Call close() before
   * Backend::stop() when deterministic logger removal and file closure are required.
   */
  ~CsvWriter() { frontend_t::remove_logger(_logger); }

  /**
   * Appends a row to the CSV file. This function is also thread safe.
   *
   * @note Fields are written verbatim. A string field containing a comma, double quote or line
   * break corrupts the CSV structure; pass such fields through utility::csv_escape_field() from
   * quill/Utility.h or sanitize them beforehand.
   *
   * @param fields The fields to append to the CSV row.
   */
  template <typename... Args>
  void append_row(Args&&... fields)
  {
    _throw_if_closed("append_row()");
    _logger->template log_statement<false>(&_line_metadata, static_cast<Args&&>(fields)...);
  }

  /**
   * Writes the csv header
   */
  void write_header()
  {
    _throw_if_closed("write_header()");
    _logger->template log_statement<false>(&_header_metadata, TCsvSchema::header);
  }

  /**
   * Writes the csv header to the specified file
   * @param file file to write
   */
  void write_header(FileEventNotifierHandle file) { _write_header(file); }

  /**
   * @brief Removes the logger synchronously and closes the underlying file sink before returning.
   * @note This function must only be used while the backend worker is running.
   *
   * After calling close(), this CsvWriter instance must no longer be used.
   */
  void close()
  {
    frontend_t::remove_logger_blocking(_logger);
    _logger = nullptr;
  }

  /**
   * @brief Flushes the log to ensure all data is written to the file.
   * This method will block the caller thread until the file is flushed, ensuring that all data are flushed to the file
   */
  void flush()
  {
    _throw_if_closed("flush()");
    _logger->flush_log();
  }

private:
  static bool _is_append_mode(std::string const& open_mode) noexcept
  {
    return !open_mode.empty() && ((open_mode[0] == 'a') || (open_mode[0] == 'A'));
  }

  static bool _should_write_header_on_initial_file(std::shared_ptr<Sink> const& sink,
                                                   bool is_append_mode, bool should_write_header)
  {
    if (!should_write_header || !is_append_mode)
    {
      return should_write_header;
    }

#if QUILL_USE_RTTI
    auto const stream_sink = std::dynamic_pointer_cast<StreamSink>(sink);

    if (!stream_sink)
    {
      // not a file backed sink, there is no existing file to inspect
      return should_write_header;
    }
#else
    auto const stream_sink = std::static_pointer_cast<StreamSink>(sink);
#endif

    std::error_code ec;
    auto const size = fs::file_size(stream_sink->get_filename(), ec);
    return ec || (size == 0);
  }

  void _throw_if_closed(char const* api_name) const
  {
    if (QUILL_UNLIKELY(!_logger))
    {
      QUILL_THROW(QuillError{"CsvWriter::" + std::string{api_name} + " called after close()"});
    }
  }

  static void _write_header(FileEventNotifierHandle file)
  {
#if defined(_WIN32)
    DWORD bytes_written = 0;
    if (!::WriteFile(file, TCsvSchema::header, static_cast<DWORD>(std::strlen(TCsvSchema::header)),
                     &bytes_written, nullptr) ||
        (bytes_written != std::strlen(TCsvSchema::header)))
    {
      QUILL_THROW(QuillError{"CsvWriter failed to write header"});
    }

    if (!::WriteFile(file, "\n", 1, &bytes_written, nullptr) || (bytes_written != 1))
    {
      QUILL_THROW(QuillError{"CsvWriter failed to write newline"});
    }
#else
    StreamSink::safe_fwrite(TCsvSchema::header, sizeof(char), std::strlen(TCsvSchema::header), file);
    StreamSink::safe_fwrite("\n", sizeof(char), 1, file);
#endif
  }

  static constexpr MacroMetadata _header_metadata{
    "", "", "{}", nullptr, LogLevel::Info, MacroMetadata::Event::Log};

  static constexpr MacroMetadata _line_metadata{
    "", "", TCsvSchema::format, nullptr, LogLevel::Info, MacroMetadata::Event::Log};

  static std::string _make_logger_name(std::string const& base_name)
  {
    uint64_t const logger_id = _next_logger_id.fetch_add(1, std::memory_order_relaxed);
    return _logger_name_prefix + base_name + "_" + std::to_string(logger_id);
  }

  static inline std::string _logger_name_prefix{"__csv__"};
  static inline std::atomic<uint64_t> _next_logger_id{0};

  LoggerImpl<TFrontendOptions>* _logger{nullptr};
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
