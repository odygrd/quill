/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/bundled/fmt/core.h"
#include "quill/core/Attributes.h"
#include "quill/core/Common.h"
#include "quill/core/FormatBuffer.h"
#include "quill/core/LogLevel.h"
#include "quill/core/MacroMetadata.h"
#include "quill/sinks/StreamSink.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace quill
{
class JsonConsoleSink : public StreamSink
{
public:
  JsonConsoleSink() : StreamSink("stdout", nullptr) {}
  ~JsonConsoleSink() override = default;

  /**
   * @brief Logs a formatted log message to the sink.
   * @note Accessor for backend processing.
   * @param log_metadata Pointer to the macro metadata.
   * @param log_timestamp Timestamp of the log event.
   * @param thread_id ID of the thread.
   * @param thread_name Name of the thread.
   * @param logger_name Name of the logger.
   * @param log_level Log level of the message.
   * @param structured_keys_values Vector of key-value pairs for structured logging.
   * @param log_message The log message.
   */
  QUILL_ATTRIBUTE_HOT void write_log_message(MacroMetadata const* log_metadata, uint64_t log_timestamp,
                                             std::string_view thread_id, std::string_view thread_name,
                                             std::string_view logger_name, LogLevel log_level,
                                             std::vector<std::pair<std::string, std::string>> const* structured_keys_values,
                                             std::string_view log_message) override
  {
    _json_message.clear();

    _json_message.append(fmtquill::format(
      R"({{"timestamp":"{}","file_name":"{}","line":"{}","thread_id":"{}","logger":"{}","log_level":"{}","message":"{}")",
      std::to_string(log_timestamp), log_metadata->file_name(), log_metadata->line(), thread_id,
      logger_name, loglevel_to_string(log_level), log_metadata->message_format()));

    if (structured_keys_values)
    {
      for (auto const& [key, value] : *structured_keys_values)
      {
        _json_message.append(",\"");
        _json_message.append(key);
        _json_message.append("\":\"");
        _json_message.append(value);
        _json_message.append("\"");
      }
    }

    _json_message.append("}\n");

    StreamSink::write_log_message(log_metadata, log_timestamp, thread_id, thread_name, logger_name,
                                  log_level, structured_keys_values,
                                  std::string_view{_json_message.data(), _json_message.size()});
  }

private:
  FormatBuffer _json_message;
};
} // namespace quill
