/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/bundled/fmt/base.h"
#include "quill/core/Attributes.h"
#include "quill/core/FormatBuffer.h"
#include "quill/core/LogLevel.h"
#include "quill/core/MacroMetadata.h"
#include "quill/sinks/StreamSink.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

QUILL_BEGIN_NAMESPACE

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
   * @param process_id Process Id
   * @param logger_name Name of the logger.
   * @param log_level Log level of the message.
   * @param log_level_description Description of the log level.
   * @param log_level_short_code Short code representing the log level.
   * @param named_args Vector of key-value pairs of named args
   */
  QUILL_ATTRIBUTE_HOT void write_log(MacroMetadata const* log_metadata, uint64_t log_timestamp,
                                     std::string_view thread_id, std::string_view thread_name,
                                     std::string const& process_id, std::string_view logger_name, LogLevel log_level, std::string_view log_level_description,
                                     std::string_view log_level_short_code,
                                     std::vector<std::pair<std::string, std::string>> const* named_args,
                                     std::string_view, std::string_view) override
  {
    _json_message.clear();

    char const* message_format;

    if (strchr(log_metadata->message_format(), '\n') != nullptr)
    {
      // The format string contains at least one new line and that would break the json message, it needs to be removed
      _format = log_metadata->message_format();

      for (size_t pos = 0; (pos = _format.find('\n', pos)) != std::string::npos; pos++)
      {
        _format.replace(pos, 1, " ");
      }

      message_format = _format.data();
    }
    else
    {
      message_format = log_metadata->message_format();
    }

    _json_message.append(fmtquill::format(
      R"({{"timestamp":"{}","file_name":"{}","line":"{}","thread_id":"{}","logger":"{}","log_level":"{}","message":"{}")",
      std::to_string(log_timestamp), log_metadata->file_name(), log_metadata->line(), thread_id,
      logger_name, log_level_description, message_format));

    if (named_args)
    {
      for (auto const& [key, value] : *named_args)
      {
        _json_message.append(",\"");
        _json_message.append(key);
        _json_message.append("\":\"");
        _json_message.append(value);
        _json_message.append("\"");
      }
    }

    _json_message.append("}\n");

    StreamSink::write_log(log_metadata, log_timestamp, thread_id, thread_name, process_id, logger_name, log_level,
                          log_level_description, log_level_short_code, named_args, std::string_view{},
                          std::string_view{_json_message.data(), _json_message.size()});
  }

private:
  detail::FormatBuffer _json_message;
  std::string _format;
};

QUILL_END_NAMESPACE