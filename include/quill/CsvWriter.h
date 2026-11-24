/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Frontend.h"
#include "quill/core/Attributes.h"
#include "quill/core/Filesystem.h"
#include "quill/core/QuillError.h"
#include "quill/sinks/FileSink.h"
#include "quill/sinks/RotatingFileSink.h"
#include "quill/sinks/Sink.h"
#include "quill/sinks/StreamSink.h"

#ifndef QUILL_MODULE
  #include <cstdint>
  #include <cstdio>
  #include <cstring>
  #include <memory>
  #include <string>
  #include <type_traits>
  #include <utility>
#endif

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
 * A file whose sink is shared, e.g. by writers that are given the same file name, is closed when
 * the last owner of the sink releases it.
 *
 * A writer that is given a file name makes the file sink write the header, see
 * FileSinkConfig::set_file_header(): every file the sink creates starts with it, including each
 * file of a rotating sink, and a file that is appended to and already has content gets no header.
 *
 * Writers that are given the same file name share its file sink, which writes the header once
 * in every file, so writers with the same schema need no should_write_header=false and can be
 * created in any order. The sink keeps the header of the writer that created it: a later writer
 * that asks for a header that the sink does not write, e.g. after a writer with
 * should_write_header=false, throws QuillError. Writers that are given a sink log a header row
 * each instead, so set should_write_header=false for all but one of them.
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
  using clock_source_t = typename LoggerImpl<TFrontendOptions>::clock_source_t;

  CsvWriter(CsvWriter const&) = delete;
  CsvWriter& operator=(CsvWriter const&) = delete;
  CsvWriter(CsvWriter&&) = delete;
  CsvWriter& operator=(CsvWriter&&) = delete;

  /**
   * Constructs a CsvWriter object that writes to a file.
   *
   * @param filename The name of the CSV file to write to.
   * @param open_mode The mode in which to open the file: 'a' to append, the default, as the other
   * writers do, or 'w' to truncate it. A file that does not exist is created, with the header.
   * @param filename_append Option to append to the filename (None, StartDate, StartDateTime).
   * @param clock ClockSource::tsc(), ClockSource::system(), or ClockSource::custom(clock), for the
   * timestamps that order the rows. A non-default frontend clock_type requires a matching
   * user-clock reference.
   */
  explicit CsvWriter(std::string const& filename, char open_mode = 'a',
                     FilenameAppendOption filename_append = FilenameAppendOption::None,
                     clock_source_t clock = ClockSource::tsc())
  {
    FileSinkConfig sink_config{_make_file_sink_config(open_mode, filename_append)};
    _create_file_logger<FileSink>(filename, sink_config, true, clock);
  }

  /**
   * Constructs a CsvWriter object that writes to a file.
   *
   * @param filename The name of the CSV file to write to.
   * @param sink_config Configuration settings for the file sink.
   * @param should_write_header Whether to write the header at the beginning of the CSV file.
   * @param clock The clock of the timestamps that order the rows, see the constructor above.
   */
  CsvWriter(std::string const& filename, FileSinkConfig sink_config,
            bool should_write_header = true, clock_source_t clock = ClockSource::tsc())
  {
    _create_file_logger<FileSink>(filename, sink_config, should_write_header, clock);
  }

  /**
   * Constructs a CsvWriter object that writes to a rotating file.
   *
   * @param filename The name of the CSV file to write to.
   * @param sink_config Configuration settings for the file sink.
   * @param should_write_header Whether to write the header at the beginning of each CSV file.
   * @param clock The clock of the timestamps that order the rows, see the constructor above.
   */
  CsvWriter(std::string const& filename, RotatingFileSinkConfig sink_config,
            bool should_write_header = true, clock_source_t clock = ClockSource::tsc())
  {
    _create_file_logger<RotatingFileSink>(filename, sink_config, should_write_header, clock);
  }

  /**
   * The three constructors above for a file that is given by path. Use them for a path that a
   * std::string cannot spell: on Windows a std::string name is decoded with the active code
   * page, while a path built from a wide string keeps every character.
   */
  template <typename TPath, std::enable_if_t<std::is_same_v<TPath, fs::path>, int> = 0>
  explicit CsvWriter(TPath const& filename, char open_mode = 'a',
                     FilenameAppendOption filename_append = FilenameAppendOption::None,
                     clock_source_t clock = ClockSource::tsc())
  {
    FileSinkConfig sink_config{_make_file_sink_config(open_mode, filename_append)};
    _create_file_logger<FileSink>(filename, sink_config, true, clock);
  }

  /***/
  template <typename TPath, std::enable_if_t<std::is_same_v<TPath, fs::path>, int> = 0>
  CsvWriter(TPath const& filename, FileSinkConfig sink_config, bool should_write_header = true,
            clock_source_t clock = ClockSource::tsc())
  {
    _create_file_logger<FileSink>(filename, sink_config, should_write_header, clock);
  }

  /***/
  template <typename TPath, std::enable_if_t<std::is_same_v<TPath, fs::path>, int> = 0>
  CsvWriter(TPath const& filename, RotatingFileSinkConfig sink_config,
            bool should_write_header = true, clock_source_t clock = ClockSource::tsc())
  {
    _create_file_logger<RotatingFileSink>(filename, sink_config, should_write_header, clock);
  }

  /**
   * Constructs a CsvWriter object that writes to a specified sink.
   *
   * @param unique_name A unique name for this CsvWriter instance.
   * @param sink The sink to output the data to (e.g., a ConsoleSink or a user-defined Sink).
   * @param should_write_header Whether to write the header at the beginning of the CSV file.
   * @param clock The clock of the timestamps that order the rows, see the constructor above.
   */
  CsvWriter(std::string const& unique_name, std::shared_ptr<Sink> sink,
            bool should_write_header = true, clock_source_t clock = ClockSource::tsc())
  {
    _logger = frontend_t::create_or_get_logger(
      _make_logger_name(unique_name), std::move(sink),
      PatternFormatterOptions{"%(message)", "", Timezone::GmtTime}, clock);

    if (should_write_header)
    {
      _write_header_or_remove_logger();
    }
  }

  /**
   * Constructs a CsvWriter object that writes to multiple sinks.
   *
   * @param unique_name A unique name for this CsvWriter instance.
   * @param sinks A list of sinks to output the data to.
   * @param should_write_header Whether to write the header at the beginning of the CSV file.
   * @param clock The clock of the timestamps that order the rows, see the constructor above.
   */
  CsvWriter(std::string const& unique_name, std::vector<std::shared_ptr<Sink>> sinks,
            bool should_write_header = true, clock_source_t clock = ClockSource::tsc())
  {
    _logger = frontend_t::create_or_get_logger(
      _make_logger_name(unique_name), sinks, PatternFormatterOptions{"%(message)", "", Timezone::GmtTime}, clock);

    if (should_write_header)
    {
      _write_header_or_remove_logger();
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
    _logger->log_statement(&_line_metadata, static_cast<Args&&>(fields)...);
  }

  /**
   * Writes the csv header
   */
  void write_header()
  {
    _throw_if_closed("write_header()");
    _logger->log_statement(&_header_metadata, TCsvSchema::header);
  }

  /**
   * Writes the csv header to the specified file
   * @param file file to write
   */
  void write_header(FileEventNotifierHandle file)
  {
#if defined(_WIN32)
    DWORD bytes_written = 0;
    if (!::WriteFile(file, TCsvSchema::header, static_cast<DWORD>(std::strlen(TCsvSchema::header)),
                     &bytes_written, nullptr) ||
        (bytes_written != std::strlen(TCsvSchema::header)))
    {
      QUILL_THROW_OR_RETURN(QuillError{"CsvWriter failed to write header"});
    }

    if (!::WriteFile(file, "\n", 1, &bytes_written, nullptr) || (bytes_written != 1))
    {
      QUILL_THROW_OR_RETURN(QuillError{"CsvWriter failed to write newline"});
    }
#else
    StreamSink::safe_fwrite(TCsvSchema::header, sizeof(char), std::strlen(TCsvSchema::header), file);
    StreamSink::safe_fwrite("\n", sizeof(char), 1, file);
#endif
  }

  /**
   * @brief Removes the logger of this writer synchronously. The file is closed before this
   * function returns, unless something else still holds its sink, e.g. another writer of the
   * same file: the file is then closed when the last owner of the sink releases it.
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
  /** The configuration of the file sink of a writer that is given an open mode instead of one */
  QUILL_NODISCARD static FileSinkConfig _make_file_sink_config(char open_mode, FilenameAppendOption filename_append)
  {
    FileSinkConfig sink_config;
    sink_config.set_open_mode(open_mode);
    sink_config.set_filename_append_option(filename_append);
    return sink_config;
  }

  /**
   * Creates the logger of a writer that is given a file name. The file sink writes the header
   * when it creates a file, so that it is also the first line of each file after a rotation.
   */
  template <typename TFileSink, typename TFilename, typename TFileSinkConfig>
  void _create_file_logger(TFilename const& filename, TFileSinkConfig& sink_config,
                           bool should_write_header, clock_source_t clock)
  {
    if (should_write_header)
    {
      sink_config.set_file_header(std::string{TCsvSchema::header} + "\n");
    }

    // Writers that are given the same file name share its sink
    std::shared_ptr<TFileSink> sink = frontend_t::template create_or_get_sink<TFileSink>(filename, sink_config);

    if (should_write_header && (sink->config().file_header() != sink_config.file_header()))
    {
      // The file has a sink already, which keeps the configuration that another writer or the
      // application created it with, whatever its type. A header that this writer logged would
      // start the file that is open and none that the sink opens later
      QUILL_THROW(QuillError{
        "The file sink of a CsvWriter that writes the header must be created by that writer, "
        "before the writers that share the file with should_write_header=false: " +
        detail::file_sink_name_string(filename)});
    }

    _logger = frontend_t::create_or_get_logger(
      _make_logger_name(detail::file_sink_name_string(filename)), std::move(sink),
      PatternFormatterOptions{"%(message)", "", Timezone::GmtTime}, clock);
  }

  /**
   * Logs the header of a writer that is given a sink. The destructor does not run for a
   * constructor that throws, e.g. for a header that is larger than a bounded queue, so the
   * logger is removed here
   */
  void _write_header_or_remove_logger()
  {
    QUILL_TRY { write_header(); }
#if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH_ALL()
    {
      frontend_t::remove_logger(_logger);
      throw;
    }
#endif
  }

  void _throw_if_closed(char const* api_name) const
  {
    if (QUILL_UNLIKELY(!_logger))
    {
      QUILL_THROW(QuillError{"CsvWriter::" + std::string{api_name} + " called after close()"});
    }
  }

  static constexpr MacroMetadata _header_metadata{
    "", "", "{}", nullptr, LogLevel::Info, MacroMetadata::Event::Log};

  static constexpr MacroMetadata _line_metadata{
    "", "", TCsvSchema::format, nullptr, LogLevel::Info, MacroMetadata::Event::Log};

  static std::string _make_logger_name(std::string const& base_name)
  {
    // Every CsvWriter specialization, in every shared library of the application, draws its id
    // from one counter, so writers with different schemas on the same file get different logger
    // names and closing one cannot remove the logger of another.
    // The prefix is a literal: a static std::string member of a class template is not yet
    // initialised when a writer at namespace scope is constructed.
    uint64_t const logger_id = detail::LoggerManager::instance().next_unique_logger_id();
    return "__csv__" + base_name + "_" + std::to_string(logger_id);
  }

  LoggerImpl<TFrontendOptions>* _logger{nullptr};
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
