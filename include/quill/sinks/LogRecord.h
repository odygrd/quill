/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/LogLevel.h"

#ifndef QUILL_MODULE
  #include <cstdint>
  #include <string>
  #include <string_view>
  #include <utility>
  #include <vector>
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

/** Forward Declarations **/
class MacroMetadata;

/**
 * A log record, passed to Sink::write_log() and Filter::filter() on the backend thread.
 *
 * The views and pointers refer to backend-owned storage and are valid only during the call.
 * A sink that keeps a record, e.g. to write it from another thread, must copy what it needs.
 */
struct LogRecord
{
  /** Source location, format string and tags of the log statement */
  MacroMetadata const* log_metadata{nullptr};

  /** Timestamp in nanoseconds since epoch */
  uint64_t log_timestamp{0};

  std::string_view thread_id{};
  std::string_view thread_name{};
  std::string_view process_id{};
  std::string_view logger_name{};
  LogLevel log_level{LogLevel::None};
  std::string_view log_level_description{};
  std::string_view log_level_short_code{};

  /** The named arguments as key/value pairs, or nullptr when the format string has none */
  std::vector<std::pair<std::string, std::string>> const* named_args{nullptr};

  /** The formatted message */
  std::string_view log_message{};

  /** The message formatted with the sink's pattern */
  std::string_view log_statement{};

  /** The logging thread's MDC fields formatted with BackendOptions::mdc_format_pattern, or empty */
  std::string_view mdc{};
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
