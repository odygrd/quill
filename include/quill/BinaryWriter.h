/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Frontend.h"
#include "quill/core/Attributes.h"
#include "quill/core/FrontendOptions.h"
#include "quill/core/LogLevel.h"
#include "quill/core/MacroMetadata.h"
#include "quill/core/QuillError.h"
#include "quill/sinks/FileSink.h"
#include "quill/sinks/RotatingFileSink.h"
#include "quill/sinks/Sink.h"

#ifndef QUILL_MODULE
  #include <cstddef>
  #include <cstdint>
  #include <memory>
  #include <string>
  #include <utility>
  #include <vector>
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

/**
 * @brief Writes binary records to a file asynchronously.
 *
 * The bytes of a record are copied to the queue of the calling thread, and the backend worker
 * thread writes them to the file as they are, see LOG_BINARY. The writer creates the file sink
 * and a logger of its own for it, so the file has the records and nothing else.
 *
 * The sink configuration has the options of the file: FileSinkConfig::set_file_header() sets
 * bytes that start every file, e.g. a magic number, and
 * FileSinkConfig::set_binary_record_framing() makes the sink write the size of each record, or
 * its timestamp and size, before its bytes, for records of different sizes.
 *
 * Call close() before Backend::stop() when deterministic logger removal and file closure
 * are required. The destructor performs best-effort asynchronous cleanup and does not block.
 * A file whose sink is shared, e.g. by writers that are given the same file name, is closed when
 * the last owner of the sink releases it.
 *
 * @tparam TFrontendOptions Custom frontend options if they are used application-wide.
 * quill::BinaryWriter is the writer for quill::FrontendOptions.
 */
template <typename TFrontendOptions>
class BinaryWriterImpl
{
public:
  using frontend_t = FrontendImpl<TFrontendOptions>;
  using clock_source_t = typename LoggerImpl<TFrontendOptions>::clock_source_t;

  BinaryWriterImpl(BinaryWriterImpl const&) = delete;
  BinaryWriterImpl& operator=(BinaryWriterImpl const&) = delete;
  BinaryWriterImpl(BinaryWriterImpl&&) = delete;
  BinaryWriterImpl& operator=(BinaryWriterImpl&&) = delete;

  /**
   * Constructs a writer to a file.
   *
   * @param filename The name of the file to write to.
   * @param sink_config Configuration settings for the file sink. By default it appends to the file.
   * @param clock ClockSource::tsc(), ClockSource::system(), or ClockSource::custom(clock), for the
   * timestamps of the records. A non-default frontend clock_type requires a matching user-clock
   * reference.
   */
  explicit BinaryWriterImpl(std::string const& filename, FileSinkConfig const& sink_config = FileSinkConfig{},
                            clock_source_t clock = ClockSource::tsc())
  {
    _create_logger(filename, _create_file_sink<FileSink>(filename, sink_config), clock);
  }

  /**
   * Constructs a writer to a rotating file. A record is not split between two files.
   *
   * @param filename The name of the file to write to.
   * @param sink_config Configuration settings for the rotating file sink.
   * @param clock The clock of the timestamps of the records, see the constructor above.
   */
  BinaryWriterImpl(std::string const& filename, RotatingFileSinkConfig const& sink_config,
                   clock_source_t clock = ClockSource::tsc())
  {
    _create_logger(filename, _create_file_sink<RotatingFileSink>(filename, sink_config), clock);
  }

  /**
   * Constructs a writer to a specified sink.
   *
   * @param unique_name A unique name for this writer.
   * @param sink The sink to output the records to, e.g. a user-defined sink that implements
   * Sink::write_binary().
   * @param clock The clock of the timestamps of the records, see the constructor above.
   */
  BinaryWriterImpl(std::string const& unique_name, std::shared_ptr<Sink> sink,
                   clock_source_t clock = ClockSource::tsc())
  {
    _create_logger(unique_name, std::move(sink), clock);
  }

  /**
   * Constructs a writer to several sinks, as CsvWriter takes them. Each sink gets every record.
   *
   * @param unique_name A unique name for this writer.
   * @param sinks The sinks to output the records to, e.g. user-defined sinks that implement
   * Sink::write_binary().
   * @param clock The clock of the timestamps of the records, see the constructor above.
   */
  BinaryWriterImpl(std::string const& unique_name, std::vector<std::shared_ptr<Sink>> sinks,
                   clock_source_t clock = ClockSource::tsc())
  {
    _create_logger(unique_name, std::move(sinks), clock);
  }

  /**
   * The destructor performs best-effort asynchronous cleanup. Call close() before
   * Backend::stop() when deterministic logger removal and file closure are required.
   */
  ~BinaryWriterImpl() { frontend_t::remove_logger(_logger); }

  /**
   * Writes a record. This function is thread safe.
   *
   * The bytes are copied, so `data` may be changed or destroyed after it returns.
   *
   * @param data the bytes of the record
   * @param size the number of bytes
   *
   * @return true if the record is written to the queue, false if it is dropped
   */
  QUILL_ATTRIBUTE_HOT bool write(void const* data, size_t size)
  {
    _throw_if_closed("write()");
    return _logger->log_binary(&_record_metadata, data, size);
  }

  /**
   * Writes a record that is given in two parts, e.g. a header that the caller builds for data it
   * has received: the bytes of the header followed by the data. They are one record, so nothing
   * comes between them in the file, also when it is rotated, and the data is copied once.
   * This function is thread safe.
   *
   * @param header the bytes that the record starts with
   * @param header_size the number of bytes of the header
   * @param data the bytes that follow the header
   * @param size the number of bytes of the data
   *
   * @return true if the record is written to the queue, false if it is dropped
   */
  QUILL_ATTRIBUTE_HOT bool write(void const* header, size_t header_size, void const* data, size_t size)
  {
    _throw_if_closed("write()");
    return _logger->log_binary(&_record_metadata, header, header_size, data, size);
  }

  /**
   * @brief Removes the logger of this writer synchronously. The file is closed before this
   * function returns, unless something else still holds its sink, e.g. another writer of the
   * same file: the file is then closed when the last owner of the sink releases it.
   * @note This function must only be used while the backend worker is running.
   *
   * After calling close(), this writer must no longer be used.
   */
  void close()
  {
    frontend_t::remove_logger_blocking(_logger);
    _logger = nullptr;
  }

  /**
   * @brief Blocks the caller thread until the records written so far are flushed to the file.
   */
  void flush()
  {
    _throw_if_closed("flush()");
    _logger->flush_log();
  }

private:
  /**
   * Creates the file sink, or gets the one that the file has: writers that are given the same
   * file name share its sink. A shared sink writes the records of this writer as this
   * configuration does: the file header and the framing of the records are the same.
   */
  template <typename TFileSink, typename TSinkConfig>
  QUILL_NODISCARD static std::shared_ptr<Sink> _create_file_sink(std::string const& filename,
                                                                 TSinkConfig const& sink_config)
  {
    std::shared_ptr<TFileSink> sink = frontend_t::template create_or_get_sink<TFileSink>(filename, sink_config);

    // A sink that exists for the file keeps the configuration that it was created with
    if ((sink->config().file_header() != sink_config.file_header()) ||
        (sink->config().binary_record_framing() != sink_config.binary_record_framing()))
    {
      QUILL_THROW(QuillError{
        "The file already has a sink that does not write the records of this writer as its "
        "configuration asks, e.g. one with another file header or framing: " +
        filename});
    }

    return sink;
  }

  void _create_logger(std::string const& base_name, std::shared_ptr<Sink> sink, clock_source_t clock)
  {
    std::vector<std::shared_ptr<Sink>> sinks;
    sinks.push_back(std::move(sink));
    _create_logger(base_name, std::move(sinks), clock);
  }

  void _create_logger(std::string const& base_name, std::vector<std::shared_ptr<Sink>> sinks, clock_source_t clock)
  {
    // Each writer owns its logger. Disable text logging before other threads can look it up.
    auto& logger_manager = detail::LoggerManager::instance();
    uint64_t const logger_id = logger_manager.next_unique_logger_id();
    _logger =
      static_cast<typename frontend_t::logger_t*>(logger_manager.create_logger<typename frontend_t::logger_t>(
        "__binary__" + base_name + "_" + std::to_string(logger_id), std::move(sinks),
        PatternFormatterOptions{}, clock.source_type(), clock.get(), LogLevel::None));
  }

  void _throw_if_closed(char const* api_name) const
  {
    if (QUILL_UNLIKELY(!_logger))
    {
      QUILL_THROW(QuillError{"BinaryWriter::" + std::string{api_name} + " called after close()"});
    }
  }

  static constexpr MacroMetadata _record_metadata{
    "", "", "", nullptr, LogLevel::None, MacroMetadata::Event::Binary};

  LoggerImpl<TFrontendOptions>* _logger{nullptr};
};

using BinaryWriter = BinaryWriterImpl<FrontendOptions>;

QUILL_END_EXPORT

QUILL_END_NAMESPACE
