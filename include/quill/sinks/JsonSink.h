/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/bundled/fmt/base.h"
#include "quill/core/Attributes.h"
#include "quill/core/Filesystem.h"
#include "quill/core/LogLevel.h"
#include "quill/core/MacroMetadata.h"
#include "quill/sinks/FileSink.h"
#include "quill/sinks/StreamSink.h"

#include "quill/bundled/fmt/format.h"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

namespace detail
{
template <typename TBase>
class JsonSink : public TBase
{
public:
  using base_type = TBase;

  /** Inherit base constructors **/
  using base_type::base_type;

  ~JsonSink() override = default;

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
   * @param log_message log message
   * @param log_statement log statement
   */
  QUILL_ATTRIBUTE_HOT void write_log(MacroMetadata const* log_metadata, uint64_t log_timestamp,
                                     std::string_view thread_id, std::string_view thread_name,
                                     std::string const& process_id, std::string_view logger_name,
                                     LogLevel log_level, std::string_view log_level_description,
                                     std::string_view log_level_short_code,
                                     std::vector<std::pair<std::string, std::string>> const* named_args,
                                     std::string_view log_message, std::string_view log_statement) override
  {
    if (!_json_message_ready)
    {
      char const* message_format = log_metadata->message_format();

      if (strchr(message_format, '\n') != nullptr)
      {
        // The format string contains at least one new line and that would break the json message, it needs to be removed
        _format = message_format;

        for (size_t pos = 0; (pos = _format.find('\n', pos)) != std::string::npos; pos++)
        {
          _format.replace(pos, 1, " ");
        }

        // we do not want newlines in the json message, use the modified message_format
        message_format = _format.data();
      }

      _json_message.clear();

      generate_json_message(log_metadata, log_timestamp, thread_id, thread_name, process_id,
                            logger_name, log_level, log_level_description, log_level_short_code,
                            named_args, log_message, log_statement, message_format);

      _json_message.append(std::string_view{"}\n"});
    }

    _json_message_ready = false;

    base_type::write_log(log_metadata, log_timestamp, thread_id, thread_name, process_id, logger_name, log_level,
                         log_level_description, log_level_short_code, named_args, std::string_view{},
                         std::string_view{_json_message.data(), _json_message.size()});
  }

  /**
   * Generates a JSON-formatted log message.
   *
   * This function creates the default JSON structure for log messages, including the timestamp,
   * file name, line number, thread information, logger name, log level, and message content.
   *
   * It is designed to be customizable by overriding in derived classes. Users can provide their own
   * implementation to generate a log message in a custom format or to include additional fields.
   */
  QUILL_ATTRIBUTE_HOT virtual void generate_json_message(
    MacroMetadata const* log_metadata, uint64_t log_timestamp, std::string_view thread_id,
    std::string_view /** thread_name **/, std::string const& /** process_id **/,
    std::string_view logger_name, LogLevel /** log_level **/,
    std::string_view log_level_description, std::string_view /** log_level_short_code **/,
    std::vector<std::pair<std::string, std::string>> const* named_args,
    std::string_view /** log_message **/, std::string_view /** log_statement **/, char const* message_format)
  {
    _json_message.append(std::string_view{"{\"timestamp\":\""});
    _append_json_escaped(_json_message, std::to_string(log_timestamp));
    _json_message.append(std::string_view{"\",\"file_name\":\""});
    _append_json_escaped(_json_message, log_metadata->file_name());
    _json_message.append(std::string_view{"\",\"line\":\""});
    _append_json_escaped(_json_message, log_metadata->line());
    _json_message.append(std::string_view{"\",\"thread_id\":\""});
    _append_json_escaped(_json_message, thread_id);
    _json_message.append(std::string_view{"\",\"logger\":\""});
    _append_json_escaped(_json_message, logger_name);
    _json_message.append(std::string_view{"\",\"log_level\":\""});
    _append_json_escaped(_json_message, log_level_description);
    _json_message.append(std::string_view{"\",\"message\":\""});
    _append_json_escaped(_json_message, message_format);
    _json_message.append(std::string_view{"\""});

    // Add args as key-values
    if (named_args)
    {
      for (auto const& [key, value] : *named_args)
      {
        _json_message.append(std::string_view{",\""});

        if (_is_reserved_json_key(key))
        {
          std::string suffixed_key;
          size_t suffix{1};
          bool key_exists;

          do
          {
            suffixed_key = key;
            suffixed_key += '_';
            suffixed_key += std::to_string(suffix++);

            key_exists = false;
            for (auto const& named_arg : *named_args)
            {
              if (named_arg.first == suffixed_key)
              {
                key_exists = true;
                break;
              }
            }
          } while (key_exists);

          _append_json_escaped(_json_message, suffixed_key);
        }
        else
        {
          _append_json_escaped(_json_message, key);
        }

        _json_message.append(std::string_view{"\":\""});
        _append_json_escaped(_json_message, value);
        _json_message.append(std::string_view{"\""});
      }
    }
  }

protected:
  QUILL_NODISCARD size_t estimate_write_size(MacroMetadata const* log_metadata, uint64_t log_timestamp,
                                             std::string_view thread_id, std::string_view thread_name,
                                             std::string const& process_id, std::string_view logger_name,
                                             LogLevel log_level, std::string_view log_level_description,
                                             std::string_view log_level_short_code,
                                             std::vector<std::pair<std::string, std::string>> const* named_args,
                                             std::string_view log_message, std::string_view log_statement) override
  {
    _json_message.clear();

    char const* message_format = log_metadata->message_format();
    if (strchr(message_format, '\n') != nullptr)
    {
      _format = message_format;

      for (size_t pos = 0; (pos = _format.find('\n', pos)) != std::string::npos; pos++)
      {
        _format.replace(pos, 1, " ");
      }

      message_format = _format.data();
    }

    generate_json_message(log_metadata, log_timestamp, thread_id, thread_name, process_id,
                          logger_name, log_level, log_level_description, log_level_short_code,
                          named_args, log_message, log_statement, message_format);

    _json_message.append(std::string_view{"}\n"});
    _json_message_ready = true;
    return _json_message.size();
  }

  void discard_write_estimate() noexcept { _json_message_ready = false; }

  QUILL_NODISCARD static bool _is_reserved_json_key(std::string_view key) noexcept
  {
    return (key == "timestamp") || (key == "file_name") || (key == "line") ||
      (key == "thread_id") || (key == "logger") || (key == "log_level") || (key == "message");
  }

  static void _append_json_escaped(fmtquill::memory_buffer& out, std::string_view value)
  {
    // Pre-computed escape table for control characters (0x00..0x1F). Each entry is the 6-byte
    // \uXXXX form. Faster and locale-safe compared to per-byte snprintf.
    static constexpr char control_escape_table[32][7] = {
      "\\u0000", "\\u0001", "\\u0002", "\\u0003", "\\u0004", "\\u0005", "\\u0006", "\\u0007",
      "\\u0008", "\\u0009", "\\u000A", "\\u000B", "\\u000C", "\\u000D", "\\u000E", "\\u000F",
      "\\u0010", "\\u0011", "\\u0012", "\\u0013", "\\u0014", "\\u0015", "\\u0016", "\\u0017",
      "\\u0018", "\\u0019", "\\u001A", "\\u001B", "\\u001C", "\\u001D", "\\u001E", "\\u001F"};

    size_t const size = value.size();
    char const* const data = value.data();

    for (size_t i = 0; i < size; ++i)
    {
      unsigned char const c = static_cast<unsigned char>(data[i]);
      switch (c)
      {
      case '"':
        out.append(std::string_view{"\\\""});
        break;
      case '\\':
        out.append(std::string_view{"\\\\"});
        break;
      case '\b':
        out.append(std::string_view{"\\b"});
        break;
      case '\f':
        out.append(std::string_view{"\\f"});
        break;
      case '\n':
        out.append(std::string_view{"\\n"});
        break;
      case '\r':
        out.append(std::string_view{"\\r"});
        break;
      case '\t':
        out.append(std::string_view{"\\t"});
        break;
      // U+2028 (LINE SEPARATOR) and U+2029 (PARAGRAPH SEPARATOR) are valid JSON characters
      // but they break some JavaScript consumers that treat JSON as JS source. Escape the
      // 3-byte UTF-8 sequences E2 80 A8 and E2 80 A9 when we see them.
      case 0xE2:
        if (i + 2 < size && static_cast<unsigned char>(data[i + 1]) == 0x80 &&
            (static_cast<unsigned char>(data[i + 2]) == 0xA8 || static_cast<unsigned char>(data[i + 2]) == 0xA9))
        {
          out.append(static_cast<unsigned char>(data[i + 2]) == 0xA8 ? std::string_view{"\\u2028"}
                                                                     : std::string_view{"\\u2029"});
          i += 2;
        }
        else
        {
          out.push_back(static_cast<char>(c));
        }
        break;
      default:
        if (c < 0x20)
        {
          out.append(std::string_view{control_escape_table[c], 6});
        }
        else
        {
          out.push_back(static_cast<char>(c));
        }
        break;
      }
    }
  }

  fmtquill::memory_buffer _json_message;
  std::string _format;
  bool _json_message_ready{false};
};
} // namespace detail

QUILL_END_EXPORT

/**
 * JSON File Sink
 */
QUILL_BEGIN_EXPORT
class JsonFileSink : public detail::JsonSink<FileSink>
{
public:
  JsonFileSink(fs::path const& filename, FileSinkConfig const& config,
               FileEventNotifier file_event_notifier = FileEventNotifier{}, bool do_fopen = true,
               std::chrono::system_clock::time_point start_time = std::chrono::system_clock::now())
    : detail::JsonSink<FileSink>(filename, static_cast<FileSinkConfig const&>(config),
                                 std::move(file_event_notifier), do_fopen, start_time)
  {
  }

  ~JsonFileSink() override = default;
};

/**
 * JSON Console Sink
 */
class JsonConsoleSink : public detail::JsonSink<StreamSink>
{
public:
  JsonConsoleSink() : detail::JsonSink<StreamSink>("stdout", nullptr) {}
  ~JsonConsoleSink() override = default;
};
QUILL_END_EXPORT

QUILL_END_NAMESPACE
