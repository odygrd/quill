/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Frontend.h"
#include "quill/core/Attributes.h"
#include "quill/core/LoggerBase.h"
#include "quill/sinks/FileSink.h"
#include "quill/sinks/Sink.h"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

QUILL_BEGIN_NAMESPACE

/**
 * @brief A CSV writer class for asynchronous logging of CSV files.
 *
 * This class facilitates the asynchronous logging of CSV files, where formatting
 * and I/O operations are handled by the backend worker thread.
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
  /**
   * Constructs a CsvWriter object that writes to a file.
   *
   * @param filename The name of the CSV file to write to.
   * @param open_mode The mode in which to open the file ('w' for write, 'a' for append).
   * @param filename_append Option to append to the filename (None, Date, DateTime).
   */
  explicit CsvWriter(std::string const& filename, char open_mode = 'w',
                     FilenameAppendOption filename_append = FilenameAppendOption::None)
  {
    _logger = Frontend::create_or_get_logger(
      filename + "_csv",
      Frontend::create_or_get_sink<FileSink>(filename,
                                             [open_mode, filename_append]()
                                             {
                                               FileSinkConfig cfg;
                                               cfg.set_open_mode(open_mode);
                                               cfg.set_filename_append_option(filename_append);
                                               return cfg;
                                             }()),
      quill::PatternFormatterOptions{"%(message)", "", Timezone::GmtTime});

    _logger->template log_statement<false>(quill::LogLevel::None, &header_metadata, TCsvSchema::header);
  }

  /**
   * Constructs a CsvWriter object that writes to a specified sink.
   *
   * @param unique_name A unique name for this CsvWriter instance.
   * @param sink The sink to output the data to (e.g., a ConsoleSink or a user-defined Sink).
   */
  CsvWriter(std::string const& unique_name, std::shared_ptr<Sink> sink)
  {
    _logger =
      Frontend::create_or_get_logger(unique_name + "_csv", std::move(sink),
                                     quill::PatternFormatterOptions{"%(message)", "", Timezone::GmtTime});

    _logger->template log_statement<false>(quill::LogLevel::None, &header_metadata, TCsvSchema::header);
  }

  /**
   * Constructs a CsvWriter object that writes to multiple sinks.
   *
   * @param unique_name A unique name for this CsvWriter instance.
   * @param sinks An initializer list of sinks to output the data to.
   */
  CsvWriter(std::string const& unique_name, std::initializer_list<std::shared_ptr<Sink>> sinks)
  {
    _logger = Frontend::create_or_get_logger(
      unique_name + "_csv", sinks, quill::PatternFormatterOptions{"%(message)", "", Timezone::GmtTime});

    _logger->template log_statement<false>(quill::LogLevel::None, &header_metadata, TCsvSchema::header);
  }

  /**
   * Destructor for CsvWriter. Flushes the log and removes the logger.
   */
  ~CsvWriter()
  {
    _logger->flush_log();
    Frontend::remove_logger(_logger);
  }

  /**
   * Appends a row to the CSV file. This function is also thread safe.
   *
   * @param fields The fields to append to the CSV row.
   */
  template <typename... Args>
  void append_row(Args&&... fields)
  {
    _logger->template log_statement<false>(quill::LogLevel::None, &line_metadata, fields...);
  }

  /**
   * @brief Flushes the log to ensure all data is written to the file.
   * This method will block the caller thread until the file is flushed, ensuring that all data are flushed to the file
   */
  void flush() { _logger->flush_log(); }

private:
  static constexpr quill::MacroMetadata header_metadata{
    "", "", "{}", nullptr, LogLevel::Info, quill::MacroMetadata::Event::Log};

  static constexpr quill::MacroMetadata line_metadata{
    "", "", TCsvSchema::format, nullptr, LogLevel::Info, quill::MacroMetadata::Event::Log};

  LoggerImpl<TFrontendOptions>* _logger{nullptr};
};

QUILL_END_NAMESPACE