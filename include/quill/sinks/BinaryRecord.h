/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"

#ifndef QUILL_MODULE
  #include <cstddef>
  #include <cstdint>
  #include <string_view>
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

/** Forward Declarations **/
class MacroMetadata;

/**
 * A binary record, passed to Sink::write_binary() on the backend thread.
 * Like LogRecord, it is valid only during the call.
 */
struct BinaryRecord
{
  /** Metadata of the LOG_BINARY call, e.g. its source location */
  MacroMetadata const* macro_metadata{nullptr};

  /** Timestamp in nanoseconds since epoch */
  uint64_t timestamp{0};

  std::string_view thread_id{};
  std::string_view thread_name{};
  std::string_view process_id{};
  std::string_view logger_name{};

  /** The bytes that were passed to LOG_BINARY, unchanged */
  std::byte const* data{nullptr};
  size_t size{0};
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
