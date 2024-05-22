/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/backend/TimestampFormatter.h"
#include "quill/bundled/fmt/core.h"
#include "quill/bundled/fmt/format.h"
#include "quill/core/Attributes.h"
#include "quill/core/Common.h"
#include "quill/core/FormatBuffer.h"
#include "quill/core/MacroMetadata.h"
#include "quill/core/QuillError.h"

#include <array>
#include <bitset>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace quill
{
class PatternFormatter
{
  /** Public classes **/
public:
  /**
   * Stores the precision of the timestamp
   */
  enum class TimestampPrecision : uint8_t
  {
    None,
    MilliSeconds,
    MicroSeconds,
    NanoSeconds
  };

  enum Attribute : uint8_t
  {
    Time = 0,
    FileName,
    CallerFunction,
    LogLevel,
    LogLevelId,
    LineNumber,
    Logger,
    FullPath,
    ThreadId,
    ThreadName,
    ProcessId,
    SourceLocation,
    ShortSourceLocation,
    Message,
    Tags,
    StructuredKeys,
    ATTR_NR_ITEMS
  };

  /** Main PatternFormatter class **/
public:
  /**
   * Constructor for a PatterFormatter with a custom format
   * @param format_pattern format_pattern a format string.
   *
   * The following attribute names can be used with the corresponding placeholder in a %-style format string.
   * @note: The same attribute can not be used twice in the same format pattern
   *
   * %(time)                    - Human-readable timestamp representing when the log statement was created.
   * %(file_name)               - Name of the source file where the logging call was issued.
   * %(full_path)               - Full path of the source file where the logging call was issued.
   * %(caller_function)         - Name of the function containing the logging call.
   * %(log_level)               - Textual representation of the logging level for the message.
   * %(log_level_id)            - Single-letter identifier representing the logging level.
   * %(line_number)             - Line number in the source file where the logging call was issued.
   * %(logger)                  - Name of the logger used to log the call.
   * %(message)                 - The logged message itself.
   * %(thread_id)               - ID of the thread in which the logging call was made.
   * %(thread_name)             - Name of the thread. Must be set before the first log statement on that thread.
   * %(process_id)              - ID of the process in which the logging call was made.
   * %(source_location)         - Full source file path and line number as a single string.
   * %(short_source_location)   - Shortened source file name and line number as a single string.
   * %(tags)                    - Additional custom tags appended to the message when _WITH_TAGS macros are used.
   * %(structured_keys)         - Keys appended to the message. Only applicable with structured message formatting; remains empty otherwise.
   *
   * @param time_format The format of the date. Same as strftime() format with extra specifiers `%Qms` `%Qus` `Qns`
   * @param timestamp_timezone The timezone of the timestamp, local_time or gmt_time
   *
   * @throws on invalid format string
   */
  explicit PatternFormatter(
    std::string format_pattern =
      "%(time) [%(thread_id)] %(short_source_location:<28) LOG_%(log_level:<9) "
      "%(logger:<12) %(message)",
    std::string const& time_format = "%H:%M:%S.%Qns", Timezone timestamp_timezone = Timezone::LocalTime)
    : _format_pattern(std::move(format_pattern)), _timestamp_formatter(time_format, timestamp_timezone)
  {
    _set_pattern();
  }

  PatternFormatter(PatternFormatter const& other) = delete;
  PatternFormatter(PatternFormatter&& other) noexcept = delete;
  PatternFormatter& operator=(PatternFormatter const& other) = delete;
  PatternFormatter& operator=(PatternFormatter&& other) noexcept = delete;

  /**
   * Destructor
   */
  ~PatternFormatter() = default;

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::string_view format(
    uint64_t timestamp, std::string_view thread_id, std::string_view thread_name, std::string_view process_id,
    std::string_view logger, std::string_view log_level, MacroMetadata const& log_statement_metadata,
    std::vector<std::pair<std::string, std::string>> const* structured_kvs, std::string_view log_msg)
  {
    if (_fmt_format.empty())
    {
      // nothing to format when the given format is empty. This is useful e.g. in the
      // JsonFileSink if we want to skip formatting the main message
      return std::string_view{};
    }

    // clear out existing buffer
    _formatted_log_message.clear();

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
      _set_arg_val<Attribute::CallerFunction>(log_statement_metadata.caller_function());
    }

    if (_is_set_in_pattern[Attribute::LogLevel])
    {
      _set_arg_val<Attribute::LogLevel>(log_level);
    }

    if (_is_set_in_pattern[Attribute::LogLevelId])
    {
      _set_arg_val<Attribute::LogLevelId>(log_statement_metadata.log_level_id());
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
      _set_arg_val<Attribute::SourceLocation>(log_statement_metadata.source_location());
    }

    if (_is_set_in_pattern[Attribute::ShortSourceLocation])
    {
      _set_arg_val<Attribute::ShortSourceLocation>(log_statement_metadata.short_source_location());
    }

    if (_is_set_in_pattern[Attribute::StructuredKeys])
    {
      _structured_keys_buffer.clear();

      if (structured_kvs)
      {
        for (size_t i = 0; i < structured_kvs->size(); ++i)
        {
          _structured_keys_buffer += (*structured_kvs)[i].first;

          if (i != structured_kvs->size() - 1)
          {
            _structured_keys_buffer += ", ";
          }
        }
      }

      _set_arg_val<Attribute::StructuredKeys>(_structured_keys_buffer);
    }

    if (_is_set_in_pattern[Attribute::Tags])
    {
      if (log_statement_metadata.tags())
      {
        _tags.clear();
        log_statement_metadata.tags()->format(_tags);
        _set_arg_val<Attribute::Tags>(_tags);
      }
      else
      {
        _set_arg_val<Attribute::Tags>(std::string{});
      }
    }

    _set_arg_val<Attribute::Message>(log_msg);

    fmtquill::vformat_to(std::back_inserter(_formatted_log_message), _fmt_format,
                         fmtquill::basic_format_args(_args.data(), static_cast<int>(_args.size())));

    return std::string_view{_formatted_log_message.data(), _formatted_log_message.size()};
  }

  QUILL_ATTRIBUTE_HOT std::string_view format_timestamp(std::chrono::nanoseconds timestamp)
  {
    return _timestamp_formatter.format_timestamp(timestamp);
  }

  /***/
  QUILL_NODISCARD detail::TimestampFormatter const& timestamp_formatter() const noexcept
  {
    return _timestamp_formatter;
  }

  /***/
  QUILL_NODISCARD std::string const& format_pattern() const noexcept { return _format_pattern; }

private:
  void _set_pattern()
  {
    // the order we pass the arguments here must match with the order of Attribute enum
    using namespace fmtquill::literals;
    std::tie(_fmt_format, _order_index) = _generate_fmt_format_string(
      _is_set_in_pattern, _format_pattern, "time"_a = "", "file_name"_a = "",
      "caller_function"_a = "", "log_level"_a = "", "log_level_id"_a = "", "line_number"_a = "",
      "logger"_a = "", "full_path"_a = "", "thread_id"_a = "", "thread_name"_a = "",
      "process_id"_a = "", "source_location"_a = "", "short_source_location"_a = "",
      "message"_a = "", "tags"_a = "", "structured_keys"_a = "");

    _set_arg<Attribute::Time>(std::string_view("time"));
    _set_arg<Attribute::FileName>(std::string_view("file_name"));
    _set_arg<Attribute::CallerFunction>("caller_function");
    _set_arg<Attribute::LogLevel>(std::string_view("log_level"));
    _set_arg<Attribute::LogLevelId>(std::string_view("log_level_id"));
    _set_arg<Attribute::LineNumber>("line_number");
    _set_arg<Attribute::Logger>(std::string_view("logger"));
    _set_arg<Attribute::FullPath>(std::string_view("full_path"));
    _set_arg<Attribute::ThreadId>(std::string_view("thread_id"));
    _set_arg<Attribute::ThreadName>(std::string_view("thread_name"));
    _set_arg<Attribute::ProcessId>(std::string_view("process_id"));
    _set_arg<Attribute::SourceLocation>("source_location");
    _set_arg<Attribute::ShortSourceLocation>("short_source_location");
    _set_arg<Attribute::Message>(std::string_view("message"));
    _set_arg<Attribute::Tags>(std::string_view("tags"));
    _set_arg<Attribute::StructuredKeys>(std::string_view("structured_keys"));
  }

  /***/
  template <size_t I, typename T>
  void _set_arg(T const& arg)
  {
    _args[_order_index[I]] = fmtquill::detail::make_arg<fmtquill::format_context>(arg);
  }

  template <size_t I, typename T>
  void _set_arg_val(T const& arg)
  {
    fmtquill::detail::value<fmtquill::format_context>& value_ =
      *(reinterpret_cast<fmtquill::detail::value<fmtquill::format_context>*>(
        std::addressof(_args[_order_index[I]])));

    value_ = fmtquill::detail::arg_mapper<fmtquill::format_context>().map(arg);
  }

  /***/
  PatternFormatter::Attribute static _attribute_from_string(std::string const& attribute_name)
  {
    // don't make this static as it breaks on windows with atexit when backend worker stops
    std::unordered_map<std::string, PatternFormatter::Attribute> const attr_map = {
      {"time", PatternFormatter::Attribute::Time},
      {"file_name", PatternFormatter::Attribute::FileName},
      {"caller_function", PatternFormatter::Attribute::CallerFunction},
      {"log_level", PatternFormatter::Attribute::LogLevel},
      {"log_level_id", PatternFormatter::Attribute::LogLevelId},
      {"line_number", PatternFormatter::Attribute::LineNumber},
      {"logger", PatternFormatter::Attribute::Logger},
      {"full_path", PatternFormatter::Attribute::FullPath},
      {"thread_id", PatternFormatter::Attribute::ThreadId},
      {"thread_name", PatternFormatter::Attribute::ThreadName},
      {"process_id", PatternFormatter::Attribute::ProcessId},
      {"source_location", PatternFormatter::Attribute::SourceLocation},
      {"short_source_location", PatternFormatter::Attribute::ShortSourceLocation},
      {"message", PatternFormatter::Attribute::Message},
      {"tags", PatternFormatter::Attribute::Tags},
      {"structured_keys", PatternFormatter::Attribute::StructuredKeys}};

    auto const search = attr_map.find(attribute_name);

    if (QUILL_UNLIKELY(search == attr_map.cend()))
    {
      QUILL_THROW(QuillError{
        std::string{"Attribute enum value does not exist for attribute with name " + attribute_name}});
    }

    return search->second;
  }

  /***/
  template <size_t, size_t>
  constexpr void _store_named_args(std::array<fmtquill::detail::named_arg_info<char>, PatternFormatter::Attribute::ATTR_NR_ITEMS>&)
  {
  }

  /***/
  template <size_t Idx, size_t NamedIdx, typename Arg, typename... Args>
  constexpr void _store_named_args(
    std::array<fmtquill::detail::named_arg_info<char>, PatternFormatter::Attribute::ATTR_NR_ITEMS>& named_args_store,
    const Arg& arg, const Args&... args)
  {
    named_args_store[NamedIdx] = {arg.name, Idx};
    _store_named_args<Idx + 1, NamedIdx + 1>(named_args_store, args...);
  }

  /**
   * Convert the pattern to fmt format string and also populate the order index array
   * e.g. given :
   *   "%(time) [%(thread_id)] %(file_name):%(line_number) %(log_level:<12) %(logger) - "
   *
   * is changed to :
   *  {} [{}] {}:{} {:<12} {} -
   *
   *  with a order index of :
   *  i: 0 order idx[i] is: 0 - %(time)
   *  i: 1 order idx[i] is: 2 - %(file_name)
   *  i: 2 order idx[i] is: 10 - empty
   *  i: 3 order idx[i] is: 4 - %(log_level)
   *  i: 4 order idx[i] is: 10 - empty
   *  i: 5 order idx[i] is: 3 - %(line_number)
   *  i: 6 order idx[i] is: 5 - %(logger)
   *  i: 7 order idx[i] is: 10 - empty
   *  i: 8 order idx[i] is: 1 - %(thread_id)
   *  i: 9 order idx[i] is: 10 - empty
   *  i: 10 order idx[i] is: 10 - empty
   * @tparam Args Args
   * @param is_set_in_pattern is set in pattern
   * @param pattern pattern
   * @param args args
   * @return process_id pattern
   */
  template <typename... Args>
  QUILL_NODISCARD std::pair<std::string, std::array<size_t, PatternFormatter::Attribute::ATTR_NR_ITEMS>> _generate_fmt_format_string(
    std::bitset<PatternFormatter::Attribute::ATTR_NR_ITEMS>& is_set_in_pattern, std::string pattern,
    Args const&... args)
  {
    // Attribute enum and the args we are passing here must be in sync
    static_assert(PatternFormatter::Attribute::ATTR_NR_ITEMS == sizeof...(Args));

    pattern += "\n";

    std::array<size_t, PatternFormatter::Attribute::ATTR_NR_ITEMS> order_index{};
    order_index.fill(PatternFormatter::Attribute::ATTR_NR_ITEMS - 1);

    std::array<fmtquill::detail::named_arg_info<char>, PatternFormatter::Attribute::ATTR_NR_ITEMS> named_args{};
    _store_named_args<0, 0>(named_args, args...);
    uint8_t arg_idx = 0;

    // we will replace all %(....) with {} to construct a string to pass to fmt library
    size_t arg_identifier_pos = pattern.find_first_of('%');
    while (arg_identifier_pos != std::string::npos)
    {
      if (size_t const open_paren_pos = pattern.find_first_of('(', arg_identifier_pos);
          open_paren_pos != std::string::npos && (open_paren_pos - arg_identifier_pos) == 1)
      {
        // if we found '%(' we have a matching pattern and we now need to get the closed paren
        size_t const closed_paren_pos = pattern.find_first_of(')', open_paren_pos);

        if (closed_paren_pos == std::string::npos)
        {
          QUILL_THROW(QuillError{"Invalid format pattern"});
        }

        // We have everything, get the substring, this time including '%( )'
        std::string attr = pattern.substr(arg_identifier_pos, (closed_paren_pos + 1) - arg_identifier_pos);

        // find any user format specifiers
        size_t const pos = attr.find(':');
        std::string attr_name;

        if (pos != std::string::npos)
        {
          // we found user format specifiers that we want to keep.
          // e.g. %(short_source_location:<32)

          // Get only the format specifier
          // e.g. :<32
          std::string custom_format_specifier = attr.substr(pos);
          custom_format_specifier.pop_back(); // remove the ")"

          // replace with the pattern with the correct value
          std::string value;
          value += "{";
          value += custom_format_specifier;
          value += "}";

          // e.g. {:<32}
          pattern.replace(arg_identifier_pos, attr.length(), value);

          // Get the part that is the named argument
          // e.g. short_source_location
          attr_name = attr.substr(2, pos - 2);
        }
        else
        {
          // Make the replacement.
          pattern.replace(arg_identifier_pos, attr.length(), "{}");

          // Get the part that is the named argument
          // e.g. short_source_location
          attr.pop_back(); // remove the ")"

          attr_name = attr.substr(2, attr.size());
        }

        // reorder
        int id = -1;

        for (size_t i = 0; i < PatternFormatter::Attribute::ATTR_NR_ITEMS; ++i)
        {
          if (named_args[i].name == attr_name)
          {
            id = named_args[i].id;
            break;
          }
        }

        if (id < 0)
        {
          QUILL_THROW(QuillError{"Invalid format pattern, attribute with name \"" + attr_name + "\" is invalid"});
        }

        order_index[static_cast<size_t>(id)] = arg_idx++;

        // Also set the value as used in the pattern in our bitset for lazy evaluation
        PatternFormatter::Attribute const attr_enum_value = _attribute_from_string(attr_name);
        is_set_in_pattern.set(attr_enum_value);

        // Look for the next pattern to replace
        arg_identifier_pos = pattern.find_first_of('%');
      }
      else
      {
        // search for the next '%'
        arg_identifier_pos = pattern.find_first_of('%', arg_identifier_pos + 1);
      }
    }

    return std::make_pair(pattern, order_index);
  }

private:
  std::string _format_pattern;
  std::string _fmt_format;
  std::string _tags;

  std::string _structured_keys_buffer;

  /** Each named argument in the format_pattern is mapped in order to this array **/
  std::array<size_t, Attribute::ATTR_NR_ITEMS> _order_index{};
  std::array<fmtquill::basic_format_arg<fmtquill::format_context>, Attribute::ATTR_NR_ITEMS> _args{};
  std::bitset<Attribute::ATTR_NR_ITEMS> _is_set_in_pattern;

  /** class responsible for formatting the timestamp */
  detail::TimestampFormatter _timestamp_formatter;

  /** The buffer where we store each formatted string, also stored as class member to avoid
   * re-allocations. This is mutable so we can have a format() const function **/
  FormatBuffer _formatted_log_message;
};
} // namespace quill
