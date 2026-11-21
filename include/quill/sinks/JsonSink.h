/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/bundled/fmt/base.h"
#include "quill/core/Attributes.h"
#include "quill/core/BufferUtilities.h"
#include "quill/core/Filesystem.h"
#include "quill/core/LogLevel.h"
#include "quill/core/MacroMetadata.h"
#include "quill/sinks/FileSink.h"
#include "quill/sinks/StreamSink.h"

#include "quill/bundled/fmt/format.h"

#ifndef QUILL_MODULE
  #include <algorithm>
  #include <charconv>
  #include <chrono>
  #include <cstdint>
  #include <cstring>
  #include <limits>
  #include <string>
  #include <string_view>
  #include <utility>
  #include <vector>
#endif

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
   * @brief Writes the record as a JSON line.
   * @param record the log record
   */
  QUILL_ATTRIBUTE_HOT void write_log(LogRecord const& record) override
  {
    if (!_json_message_ready)
    {
      char const* const message_format = _normalized_message_format(record);

      _json_message.clear();
      generate_json_message(record, message_format);
      append_to_buffer(_json_message, std::string_view{"}\n"});
    }

    _json_message_ready = false;

    LogRecord json_record = record;
    json_record.log_message = std::string_view{};
    json_record.log_statement = std::string_view{_json_message.data(), _json_message.size()};
    base_type::write_log(json_record);
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
  QUILL_ATTRIBUTE_HOT virtual void generate_json_message(LogRecord const& record, char const* message_format)
  {
    MacroMetadata const* const log_metadata = record.log_metadata;
    std::vector<std::pair<std::string, std::string>> const* const named_args = record.named_args;

    append_to_buffer(_json_message, std::string_view{"{\"timestamp\":\""});
    char timestamp_buffer[std::numeric_limits<uint64_t>::digits10 + 1u];
    auto const timestamp_result =
      std::to_chars(timestamp_buffer, timestamp_buffer + sizeof(timestamp_buffer), record.log_timestamp);
    append_to_buffer(
      _json_message,
      std::string_view{timestamp_buffer, static_cast<size_t>(timestamp_result.ptr - timestamp_buffer)});
    append_to_buffer(_json_message, std::string_view{"\",\"file_name\":\""});
    append_json_escaped(_json_message, log_metadata->file_name());
    append_to_buffer(_json_message, std::string_view{"\",\"line\":\""});
    append_json_escaped(_json_message, log_metadata->line());
    append_to_buffer(_json_message, std::string_view{"\",\"thread_id\":\""});
    append_json_escaped(_json_message, record.thread_id);
    append_to_buffer(_json_message, std::string_view{"\",\"logger\":\""});
    append_json_escaped(_json_message, record.logger_name);
    append_to_buffer(_json_message, std::string_view{"\",\"log_level\":\""});
    append_json_escaped(_json_message, record.log_level_description);
    append_to_buffer(_json_message, std::string_view{"\",\"message\":\""});
    append_json_escaped(_json_message, message_format);
    append_to_buffer(_json_message, std::string_view{"\""});

    // Add args as key-values
    if (named_args)
    {
      for (auto const& [key, value] : *named_args)
      {
        append_to_buffer(_json_message, std::string_view{",\""});

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

          append_json_escaped(_json_message, suffixed_key);
        }
        else
        {
          append_json_escaped(_json_message, key);
        }

        append_to_buffer(_json_message, std::string_view{"\":\""});
        append_json_escaped(_json_message, value);
        append_to_buffer(_json_message, std::string_view{"\""});
      }
    }
  }

protected:
  QUILL_NODISCARD size_t estimate_write_size(LogRecord const& record) override
  {
    _json_message.clear();

    char const* const message_format = _normalized_message_format(record);
    generate_json_message(record, message_format);

    append_to_buffer(_json_message, std::string_view{"}\n"});
    _json_message_ready = true;
    return _json_message.size();
  }

  void discard_write_estimate() noexcept { _json_message_ready = false; }

  QUILL_NODISCARD char const* _normalized_message_format(LogRecord const& record)
  {
    MacroMetadata const* const log_metadata = record.log_metadata;
    std::vector<std::pair<std::string, std::string>> const* const named_args = record.named_args;
    std::string_view const log_message = record.log_message;

    char const* const message_format = log_metadata->message_format();
    if (!log_metadata->is_rate_limited() && (strchr(message_format, '\n') == nullptr))
    {
      return message_format;
    }

    _format = message_format;

    if (log_metadata->is_rate_limited())
    {
      if (log_metadata->has_named_args())
      {
        std::string_view occurrence_key{"occurred"};
        if (named_args && !named_args->empty())
        {
          occurrence_key = named_args->back().first;
        }

        _format += " ({";
        _format.append(occurrence_key.data(), occurrence_key.size());
        _format += "}x)";
      }
      else
      {
        // Positional events intentionally have no structured argument vector carrying the
        // occurrence count. Preserve the complete rendered message instead of producing a mixed
        // manual/automatic indexing template such as "{1} {0} ({}x)".
        _format.assign(log_message.data(), log_message.size());
      }
    }

    // Keep the JSON message template on one line.
    std::replace(_format.begin(), _format.end(), '\n', ' ');

    return _format.data();
  }

  QUILL_NODISCARD static bool _is_reserved_json_key(std::string_view key) noexcept
  {
    return (key == "timestamp") || (key == "file_name") || (key == "line") ||
      (key == "thread_id") || (key == "logger") || (key == "log_level") || (key == "message");
  }

  /**
   * Appends `value` to `out` escaped for use inside a JSON string, e.g. a key or value written by
   * an overridden generate_json_message(). Quotes, backslashes and control characters are escaped.
   */
  static void append_json_escaped(fmtquill::memory_buffer& out, std::string_view value)
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

    // Bytes that need no escape, nearly all of them, are appended in runs rather than one by one
    size_t run_begin = 0;

    for (size_t i = 0; i < size; ++i)
    {
      unsigned char const c = static_cast<unsigned char>(data[i]);

      if (QUILL_LIKELY((c >= 0x20) && (c != '"') && (c != '\\') && (c != 0xE2)))
      {
        continue;
      }

      std::string_view escaped;

      switch (c)
      {
      case '"':
        escaped = std::string_view{"\\\""};
        break;
      case '\\':
        escaped = std::string_view{"\\\\"};
        break;
      case '\b':
        escaped = std::string_view{"\\b"};
        break;
      case '\f':
        escaped = std::string_view{"\\f"};
        break;
      case '\n':
        escaped = std::string_view{"\\n"};
        break;
      case '\r':
        escaped = std::string_view{"\\r"};
        break;
      case '\t':
        escaped = std::string_view{"\\t"};
        break;
      // U+2028 (LINE SEPARATOR) and U+2029 (PARAGRAPH SEPARATOR) are valid JSON characters
      // but they break some JavaScript consumers that treat JSON as JS source. Escape the
      // 3-byte UTF-8 sequences E2 80 A8 and E2 80 A9 when we see them.
      case 0xE2:
        if (i + 2 < size && static_cast<unsigned char>(data[i + 1]) == 0x80 &&
            (static_cast<unsigned char>(data[i + 2]) == 0xA8 || static_cast<unsigned char>(data[i + 2]) == 0xA9))
        {
          escaped = static_cast<unsigned char>(data[i + 2]) == 0xA8 ? std::string_view{"\\u2028"}
                                                                    : std::string_view{"\\u2029"};
        }
        break;
      default:
        escaped = std::string_view{control_escape_table[c], 6};
        break;
      }

      if (escaped.empty())
      {
        // 0xE2 that does not start a line or paragraph separator stays in the run
        continue;
      }

      append_to_buffer(out, std::string_view{data + run_begin, i - run_begin});
      append_to_buffer(out, escaped);

      // A separator escapes three bytes
      i += (c == 0xE2) ? 2u : 0u;
      run_begin = i + 1;
    }

    append_to_buffer(out, std::string_view{data + run_begin, size - run_begin});
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
