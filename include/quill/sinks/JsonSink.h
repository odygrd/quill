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
    _append_decimal(_json_message, record.log_timestamp);
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

  /**
   * @brief Ignores the record. The bytes of a binary record are not JSON, so a logger can have a
   * JSON sink for its log statements next to a file sink for its binary records.
   */
  QUILL_ATTRIBUTE_HOT void write_binary(BinaryRecord const& /* record */) override {}

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

    for (size_t i = _find_json_escape_candidate(data, 0, size); i < size;
         i = _find_json_escape_candidate(data, i, size))
    {
      unsigned char const c = static_cast<unsigned char>(data[i]);
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
        ++i;
        continue;
      }

      append_to_buffer(out, std::string_view{data + run_begin, i - run_begin});
      append_to_buffer(out, escaped);

      // A separator escapes three bytes
      i += (c == 0xE2) ? 3u : 1u;
      run_begin = i;
    }

    append_to_buffer(out, std::string_view{data + run_begin, size - run_begin});
  }

  /**
   * Appends `value` in decimal, as std::to_chars() does. Splitting off 8 digits at a time shortens
   * the chain of divisions for the 19 digits of a timestamp in nanoseconds
   */
  static void _append_decimal(fmtquill::memory_buffer& out, uint64_t value)
  {
    constexpr uint64_t chunk_divisor{100'000'000};

    char digits[std::numeric_limits<uint64_t>::digits10 + 1u];
    char* const end = digits + sizeof(digits);
    char* begin = end;

    while (value >= chunk_divisor)
    {
      uint64_t const quotient = value / chunk_divisor;
      uint32_t const chunk = static_cast<uint32_t>(value - (quotient * chunk_divisor));
      value = quotient;

      // The digits of the two halves do not depend on each other
      uint32_t const high = chunk / 10'000u;
      uint32_t const low = chunk % 10'000u;

      begin -= 8;
      std::memcpy(begin, fmtquill::detail::digits2(high / 100u), 2);
      std::memcpy(begin + 2, fmtquill::detail::digits2(high % 100u), 2);
      std::memcpy(begin + 4, fmtquill::detail::digits2(low / 100u), 2);
      std::memcpy(begin + 6, fmtquill::detail::digits2(low % 100u), 2);
    }

    uint32_t remaining = static_cast<uint32_t>(value);

    while (remaining >= 100u)
    {
      begin -= 2;
      std::memcpy(begin, fmtquill::detail::digits2(remaining % 100u), 2);
      remaining /= 100u;
    }

    if (remaining >= 10u)
    {
      begin -= 2;
      std::memcpy(begin, fmtquill::detail::digits2(remaining), 2);
    }
    else
    {
      --begin;
      *begin = static_cast<char>('0' + remaining);
    }

    append_to_buffer(out, std::string_view{begin, static_cast<size_t>(end - begin)});
  }

  /**
   * @return true if a byte of `word` may need an escape, see _find_json_escape_candidate(). The
   * high bit of a byte of `candidates` is set for a byte below 0x20 and for a byte equal to one of
   * the others, and bytes above such a byte can be set as well by a borrow
   */
  QUILL_NODISCARD static bool _has_json_escape_candidate(uint64_t word) noexcept
  {
    constexpr uint64_t ones{0x0101010101010101ull};
    constexpr uint64_t high_bits{0x8080808080808080ull};

    // Bytes equal to '"', '\\' or 0xE2 are zero in these
    uint64_t const quote = word ^ (ones * '"');
    uint64_t const backslash = word ^ (ones * '\\');
    uint64_t const separator = word ^ (ones * 0xE2u);

    uint64_t const candidates = ((word - (ones * 0x20u)) & ~word) | ((quote - ones) & ~quote) |
      ((backslash - ones) & ~backslash) | ((separator - ones) & ~separator);

    return (candidates & high_bits) != 0;
  }

  /**
   * @return the position of the first byte from `pos` that may need an escape: a control
   * character, '"', '\\', or 0xE2, which starts U+2028 and U+2029. `size` if there is none
   */
  QUILL_NODISCARD static size_t _find_json_escape_candidate(char const* data, size_t pos, size_t size) noexcept
  {
    // Check 8 bytes at a time. The bytes of a word with a candidate are checked one by one below
    for (; (pos + sizeof(uint64_t)) <= size; pos += sizeof(uint64_t))
    {
      uint64_t word;
      std::memcpy(&word, data + pos, sizeof(word));

      if (_has_json_escape_candidate(word))
      {
        break;
      }
    }

    size_t const remaining = size - pos;

    if ((remaining != 0) && (remaining < sizeof(uint64_t)) && (size >= sizeof(uint32_t)))
    {
      // Most values, e.g. a log level, a line or a number, are shorter than a word. The bytes left
      // are checked in one word: the last 8 bytes, or the first and the last 4 of a shorter value.
      // It can include bytes before `pos`, and a candidate among them only means that the bytes
      // left are checked one by one
      uint64_t word;

      if (size >= sizeof(uint64_t))
      {
        std::memcpy(&word, data + size - sizeof(uint64_t), sizeof(word));
      }
      else
      {
        uint32_t first;
        uint32_t last;
        std::memcpy(&first, data, sizeof(first));
        std::memcpy(&last, data + size - sizeof(last), sizeof(last));
        word = (static_cast<uint64_t>(last) << 32u) | first;
      }

      if (!_has_json_escape_candidate(word))
      {
        return size;
      }
    }

    for (; pos < size; ++pos)
    {
      unsigned char const c = static_cast<unsigned char>(data[pos]);

      if ((c < 0x20) || (c == '"') || (c == '\\') || (c == 0xE2))
      {
        return pos;
      }
    }

    return size;
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

protected:
  /** Binary records are ignored, so they contribute no bytes to size rotation. */
  QUILL_NODISCARD constexpr size_t binary_write_size(BinaryRecord const&) const noexcept
  {
    return 0;
  }
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
