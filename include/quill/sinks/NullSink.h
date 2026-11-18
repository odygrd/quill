/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/LogLevel.h"
#include "quill/sinks/Sink.h"

#ifndef QUILL_MODULE
  #include <cstdint>
  #include <string>
  #include <string_view>
  #include <utility>
  #include <vector>
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

/** Forward Declaration **/
class MacroMetadata;

class NullSink : public Sink
{
public:
  /**
   * @brief Discards the provided log record.
   */
  QUILL_ATTRIBUTE_HOT void write_log(LogRecord const& /* record */) override
  {
  }

  QUILL_ATTRIBUTE_HOT void flush_sink(SinkFlushReason) override {}
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
