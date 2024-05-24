/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/LogLevel.h"
#include "quill/sinks/Sink.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace quill
{
/** Forward Declaration **/
class MacroMetadata;

class NullSink : public Sink
{
public:
  QUILL_ATTRIBUTE_HOT void write_log_message(MacroMetadata const* log_metadata, uint64_t log_timestamp,
                                             std::string_view thread_id, std::string_view thread_name,
                                             std::string_view logger_name, LogLevel log_level,
                                             std::vector<std::pair<std::string, std::string>> const* named_args,
                                             std::string_view log_message) override
  {
  }

  QUILL_ATTRIBUTE_HOT void flush_sink() override {}
};
} // namespace quill
