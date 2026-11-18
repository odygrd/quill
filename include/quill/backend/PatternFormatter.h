/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/backend/TimestampFormatter.h"
#include "quill/bundled/fmt/base.h"
#include "quill/bundled/fmt/format.h"
#include "quill/core/Attributes.h"
#include "quill/core/Common.h"
#include "quill/core/MacroMetadata.h"
#include "quill/core/PatternFormatterOptions.h"
#include "quill/core/QuillError.h"

#ifndef QUILL_MODULE
  #include <array>
  #include <bitset>
  #include <chrono>
  #include <cstddef>
  #include <cstdint>
  #include <string>
  #include <string_view>
  #include <utility>
  #include <vector>
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

class PatternFormatter
{
  /** Public classes **/
public:
  enum Attribute : uint8_t
  {
    Time = 0,
    FileName,
    CallerFunction,
    LogLevel,
    LogLevelShortCode,
    LineNumber,
    Logger,
    FullPath,
    ThreadId,
    ThreadName,
    ProcessId,
    SourceLocation,
    ShortSourceLocation,
    Message,
    Mdc,
    Tags,
    NamedArgs,
    ATTR_NR_ITEMS
  };

  /** Main PatternFormatter class **/
public:
  /**
   * Constructor for a PatternFormatter with custom formatting options.
   *
   * @param options The PatternFormatterOptions object containing the formatting configuration.
   *                @see PatternFormatterOptions for detailed information on available options.
   *
   * @throws QuillError if the format string in options is invalid
   */
  explicit PatternFormatter(PatternFormatterOptions options)
    : _options(std::move(options)),
      _timestamp_formatter(_options.timestamp_pattern, _options.timestamp_timezone)
  {
    _set_pattern();
  }

  /***/
  PatternFormatter(PatternFormatter const& other) = delete;
  PatternFormatter& operator=(PatternFormatter const& other) = delete;

  /***/
  PatternFormatter& operator=(PatternFormatter&& other) noexcept = default;
  PatternFormatter(PatternFormatter&& other) noexcept = default;

  /***/
  ~PatternFormatter() = default;

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::string_view format(
    uint64_t timestamp, std::string_view thread_id, std::string_view thread_name,
    std::string_view process_id, std::string_view logger, std::string_view log_level_description,
    std::string_view log_level_short_code, MacroMetadata const& log_statement_metadata,
    std::vector<std::pair<std::string, std::string>> const* named_args, std::string_view log_msg,
    std::string_view mdc)
  {
    if (_options.format_pattern.empty())
    {
      // No formatting is needed when the format pattern is empty.
      // For example, in JsonFileSink, we can retrieve the MacroMetadata and the named arguments as
      // key-value pairs, but we do not need to format the log statement.
      return std::string_view{};
    }

    // clear out the existing buffer
    _formatted_log_message_buffer.clear();

    if (QUILL_UNLIKELY(log_msg.empty()))
    {
      // Process an empty message
      return _format(timestamp, thread_id, thread_name, process_id, logger, log_level_description,
                     log_level_short_code, log_statement_metadata, named_args, log_msg, mdc);
    }

    std::string_view formatted_log_msg;

    // Check if we need to handle multi-line formatting
    if (_options.add_metadata_to_multi_line_logs && (!named_args || named_args->empty()))
    {
      // multi line metadata only supported when named_args are not used
      size_t start = 0;

      while (start < log_msg.size())
      {
        size_t const end = log_msg.find_first_of('\n', start);

        if (end == std::string_view::npos)
        {
          // Handle the last line or a single line without a newline
          formatted_log_msg =
            _format(timestamp, thread_id, thread_name, process_id, logger, log_level_description,
                    log_level_short_code, log_statement_metadata, named_args,
                    std::string_view(log_msg.data() + start, log_msg.size() - start), mdc);
          break;
        }

        // Write the current line
        size_t line_length = end - start;
        if (_options.pattern_suffix != '\n')
        {
          // When suffix is not '\n', include the newline character in the message
          line_length++;
        }

        formatted_log_msg = _format(timestamp, thread_id, thread_name, process_id, logger, log_level_description,
                                    log_level_short_code, log_statement_metadata, named_args,
                                    std::string_view(log_msg.data() + start, line_length), mdc);
        start = end + 1;
      }
    }
    else
    {
      // Use the regular format method for single-line messages
      QUILL_ASSERT(
        !log_msg.empty(),
        "log_msg should not be empty, already checked earlier in PatternFormatter::format()");
      size_t log_message_size = log_msg.size();

      if (_options.pattern_suffix == '\n' && log_msg[log_msg.size() - 1] == '\n')
      {
        // if the log_message ends with \n we exclude it (only when using newline suffix)
        log_message_size = log_msg.size() - 1;
      }

      formatted_log_msg = _format(timestamp, thread_id, thread_name, process_id, logger,
                                  log_level_description, log_level_short_code, log_statement_metadata,
                                  named_args, std::string_view{log_msg.data(), log_message_size}, mdc);
    }

    return formatted_log_msg;
  }

  /***/
  QUILL_NODISCARD PatternFormatterOptions const& get_options() const noexcept { return _options; }

  /**
   * @return empty, or a warning that the timestamp pattern is formatted with strftime() on every
   * call because it contains a time-of-day specifier that cannot be cached, e.g. `%-H`
   */
  QUILL_NODISCARD std::string const& timestamp_pattern_warning() const noexcept
  {
    return _timestamp_formatter.warning();
  }

protected:
  /***/
  QUILL_NODISCARD static std::string_view _process_source_location_path(std::string_view source_location,
                                                                        std::string const& strip_prefix,
                                                                        bool remove_relative_paths)
  {
    std::string_view result = source_location;

    // First, handle removal of relative paths if requested
    if (remove_relative_paths)
    {
      // Remove any relative paths (e.g., relative paths can appear when using a mounted volume under docker)

      size_t const forward_slash_pos = result.rfind("../");
      size_t const backslash_pos = result.rfind("..\\");
      size_t relative_path_pos{forward_slash_pos};

      bool const use_backslash_pos = (backslash_pos != std::string_view::npos) &&
        ((relative_path_pos == std::string_view::npos) || (backslash_pos > relative_path_pos));
      if (use_backslash_pos)
      {
        relative_path_pos = backslash_pos;
      }

      if (relative_path_pos != std::string_view::npos)
      {
        result = result.substr(relative_path_pos + 3u);
      }
    }

    // Then handle prefix stripping
    if (!strip_prefix.empty())
    {
      // Find the last occurrence of the prefix in the path
      size_t prefix_pos = result.rfind(strip_prefix);

      if (prefix_pos != std::string_view::npos)
      {
        size_t after_prefix_pos = prefix_pos + strip_prefix.size();

        // If the prefix doesn't end with a separator and there is a character after the prefix
        // and that character is a separator, skip it as well
        if (after_prefix_pos < result.length() &&
            ((result[after_prefix_pos] == '/') || (result[after_prefix_pos] == '\\')))
        {
          after_prefix_pos++;
        }

        return result.substr(after_prefix_pos);
      }
      // Prefix not found, use the full path
    }

    // No prefix set or prefix not found, use the full path
    return result;
  }

private:
  /** A pre-parsed attribute and the literal text immediately preceding it. */
  struct FormatPart
  {
    size_t literal_pos{0};
    size_t literal_size{0};
    Attribute attribute{Attribute::ATTR_NR_ITEMS};
    fmtquill::format_specs specs;
    bool has_format_specs{false};
  };

  /** Attribute names, at the same index as the Attribute enum **/
  static constexpr std::array<std::string_view, Attribute::ATTR_NR_ITEMS> _attribute_names{
    "time",      "file_name",  "caller_function", "log_level",  "log_level_short_code",  "line_number",
    "logger",    "full_path",  "thread_id",       "thread_name", "process_id",           "source_location",
    "short_source_location", "message", "mdc",    "tags",       "named_args"};

  /**
   * Splits the pattern into literal text and %(attribute[:format_spec]) parts.
   * e.g. "%(time) [%(thread_id)] %(log_level:<12) - " becomes
   *   {"", Time}, {" [", ThreadId}, {"] ", LogLevel, "<12"} and the trailing literal " - "
   */
  void _set_pattern()
  {
    std::string pattern = _options.format_pattern;

    if (_options.pattern_suffix != PatternFormatterOptions::NO_SUFFIX)
    {
      pattern += _options.pattern_suffix;
    }

    size_t literal_begin = 0;
    size_t pos = 0;

    while (pos < pattern.size())
    {
      char const c = pattern[pos];
      bool const has_next = (pos + 1u) < pattern.size();

      if ((c == '{') || (c == '}'))
      {
        // Literal braces can be written directly; the doubled fmt spelling yields one brace
        _literal_buffer += c;
        pos += (has_next && (pattern[pos + 1u] == c)) ? 2u : 1u;
        continue;
      }

      if ((c != '%') || !has_next || (pattern[pos + 1u] != '('))
      {
        _literal_buffer += c;
        ++pos;
        continue;
      }

      size_t const closed_paren_pos = pattern.find(')', pos + 2u);

      if (closed_paren_pos == std::string::npos)
      {
        QUILL_THROW(QuillError{"Invalid format pattern"});
      }

      std::string_view const attr{pattern.data() + pos + 2u, closed_paren_pos - (pos + 2u)};
      size_t const colon_pos = attr.find(':');
      std::string_view const attr_name = attr.substr(0, colon_pos);

      size_t attr_index = 0;
      while ((attr_index < _attribute_names.size()) && (_attribute_names[attr_index] != attr_name))
      {
        ++attr_index;
      }

      if (attr_index == _attribute_names.size())
      {
        QUILL_THROW(QuillError{"Invalid format pattern, attribute with name \"" +
                               std::string{attr_name} + "\" is invalid"});
      }

      if (_is_set_in_pattern.test(attr_index))
      {
        QUILL_THROW(QuillError{"Invalid format pattern, attribute with name \"" +
                               std::string{attr_name} + "\" is used more than once"});
      }

      _is_set_in_pattern.set(attr_index);

      FormatPart part{literal_begin, _literal_buffer.size() - literal_begin,
                      static_cast<Attribute>(attr_index), {}, false};

      if (colon_pos != std::string_view::npos)
      {
        part.specs = _parse_format_specs(attr.substr(colon_pos + 1u));
        part.has_format_specs = true;
      }

      _format_parts.push_back(part);
      literal_begin = _literal_buffer.size();
      pos = closed_paren_pos + 1u;
    }

    _trailing_literal_pos = literal_begin;
  }

  /***/
  QUILL_NODISCARD static fmtquill::format_specs _parse_format_specs(std::string_view spec)
  {
    fmtquill::detail::dynamic_format_specs<char> specs;

    auto const parse = [&specs, spec]()
    {
      fmtquill::parse_context<char> parse_context{fmtquill::string_view{spec.data(), spec.size()}};
      char const* const end = spec.data() + spec.size();
      char const* const parsed_end = fmtquill::detail::parse_format_specs(
        spec.data(), end, specs, parse_context, fmtquill::detail::type::string_type);

      if (parsed_end != end)
      {
        fmtquill::report_error("invalid format specifier");
      }

      if (specs.dynamic())
      {
        fmtquill::report_error("width/precision is not integer");
      }
    };

#if defined(QUILL_NO_EXCEPTIONS)
    parse();
#else
    try
    {
      parse();
    }
    catch (fmtquill::format_error const& error)
    {
      QUILL_THROW(QuillError{"Invalid format pattern: " + std::string{error.what()}});
    }
#endif

    return specs;
  }

  /***/
  template <size_t I, typename T>
  void _set_arg_val(T const& arg)
  {
    _attribute_values[I] = arg;
  }

  /***/
  QUILL_ATTRIBUTE_HOT std::string_view _format(uint64_t timestamp, std::string_view thread_id,
                                               std::string_view thread_name, std::string_view process_id,
                                               std::string_view logger, std::string_view log_level_description,
                                               std::string_view log_level_short_code,
                                               MacroMetadata const& log_statement_metadata,
                                               std::vector<std::pair<std::string, std::string>> const* named_args,
                                               std::string_view log_msg, std::string_view mdc)
  {
    if (_is_set_in_pattern[Attribute::Time])
    {
      _set_arg_val<Attribute::Time>(_timestamp_formatter.format_timestamp(std::chrono::nanoseconds{timestamp}));
    }

    if (_is_set_in_pattern[Attribute::FileName])
    {
      _set_arg_val<Attribute::FileName>(log_statement_metadata.file_name());
    }

    if (_is_set_in_pattern[Attribute::CallerFunction])
    {
      std::string_view const function_name = _options.process_function_name
        ? _options.process_function_name(log_statement_metadata.caller_function())
        : std::string_view{log_statement_metadata.caller_function()};

      _set_arg_val<Attribute::CallerFunction>(function_name);
    }

    if (_is_set_in_pattern[Attribute::LogLevel])
    {
      _set_arg_val<Attribute::LogLevel>(log_level_description);
    }

    if (_is_set_in_pattern[Attribute::LogLevelShortCode])
    {
      _set_arg_val<Attribute::LogLevelShortCode>(log_level_short_code);
    }

    if (_is_set_in_pattern[Attribute::LineNumber])
    {
      _set_arg_val<Attribute::LineNumber>(log_statement_metadata.line());
    }

    if (_is_set_in_pattern[Attribute::Logger])
    {
      _set_arg_val<Attribute::Logger>(logger);
    }

    if (_is_set_in_pattern[Attribute::FullPath])
    {
      _set_arg_val<Attribute::FullPath>(log_statement_metadata.full_path());
    }

    if (_is_set_in_pattern[Attribute::ThreadId])
    {
      _set_arg_val<Attribute::ThreadId>(thread_id);
    }

    if (_is_set_in_pattern[Attribute::ThreadName])
    {
      _set_arg_val<Attribute::ThreadName>(thread_name);
    }

    if (_is_set_in_pattern[Attribute::ProcessId])
    {
      _set_arg_val<Attribute::ProcessId>(process_id);
    }

    if (_is_set_in_pattern[Attribute::SourceLocation])
    {
      _set_arg_val<Attribute::SourceLocation>(_process_source_location_path(
        log_statement_metadata.source_location(), _options.source_location_path_strip_prefix,
        _options.source_location_remove_relative_paths));
    }

    if (_is_set_in_pattern[Attribute::ShortSourceLocation])
    {
      _set_arg_val<Attribute::ShortSourceLocation>(log_statement_metadata.short_source_location());
    }

    if (_is_set_in_pattern[Attribute::NamedArgs])
    {
      _formatted_named_args_buffer.clear();

      if (named_args)
      {
        for (size_t i = 0; i < named_args->size(); ++i)
        {
          _formatted_named_args_buffer.append((*named_args)[i].first);
          _formatted_named_args_buffer.append(std::string_view{": "});
          _formatted_named_args_buffer.append((*named_args)[i].second);

          if (i != named_args->size() - 1)
          {
            _formatted_named_args_buffer.append(std::string_view{", "});
          }
        }
      }

      _set_arg_val<Attribute::NamedArgs>(
        std::string_view{_formatted_named_args_buffer.data(), _formatted_named_args_buffer.size()});
    }

    if (_is_set_in_pattern[Attribute::Mdc])
    {
      _set_arg_val<Attribute::Mdc>(mdc);
    }

    if (_is_set_in_pattern[Attribute::Tags])
    {
      if (log_statement_metadata.tags())
      {
        if (_options.process_tags)
        {
          _processed_tags = _options.process_tags(log_statement_metadata.tags());
          _set_arg_val<Attribute::Tags>(_processed_tags);
        }
        else
        {
          _set_arg_val<Attribute::Tags>(std::string_view{log_statement_metadata.tags()});
        }
      }
      else
      {
        _set_arg_val<Attribute::Tags>(std::string_view{});
      }
    }

    _set_arg_val<Attribute::Message>(log_msg);

    fmtquill::appender out{_formatted_log_message_buffer};

    for (FormatPart const& part : _format_parts)
    {
      if (part.literal_size != 0)
      {
        _formatted_log_message_buffer.append(
          std::string_view{_literal_buffer.data() + part.literal_pos, part.literal_size});
      }

      std::string_view const value = _attribute_values[part.attribute];
      if (!part.has_format_specs)
      {
        // All pattern attributes are string views, so the common case needs no fmt dispatch.
        _formatted_log_message_buffer.append(value);
      }
      else
      {
        (void)fmtquill::detail::write<char>(out, fmtquill::string_view{value.data(), value.size()},
                                            part.specs);
      }
    }

    _formatted_log_message_buffer.append(std::string_view{
      _literal_buffer.data() + _trailing_literal_pos, _literal_buffer.size() - _trailing_literal_pos});

    return std::string_view{_formatted_log_message_buffer.data(), _formatted_log_message_buffer.size()};
  }

private:
  PatternFormatterOptions _options;
  std::string _literal_buffer;
  std::vector<FormatPart> _format_parts;
  size_t _trailing_literal_pos{0};

  std::array<std::string_view, Attribute::ATTR_NR_ITEMS> _attribute_values{};
  std::bitset<Attribute::ATTR_NR_ITEMS> _is_set_in_pattern;

  /** class responsible for formatting the timestamp */
  detail::TimestampFormatter _timestamp_formatter;

  /** The buffer where we store each formatted string, also stored as class member to avoid
   * re-allocations **/
  fmtquill::basic_memory_buffer<char, 512> _formatted_log_message_buffer;
  fmtquill::basic_memory_buffer<char, 512> _formatted_named_args_buffer;
  std::string _processed_tags;
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
