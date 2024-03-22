#include "quill/PatternFormatter.h"
#include <algorithm> // for max
#include <unordered_map>

namespace quill
{

namespace
{
/***/
PatternFormatter::Attribute attribute_from_string(std::string const& attribute_name)
{
  static std::unordered_map<std::string, PatternFormatter::Attribute> const attr_map = {
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
    {"custom_tags", PatternFormatter::Attribute::CustomTags},
    {"structured_keys", PatternFormatter::Attribute::StructuredKeys}};

  auto const search = attr_map.find(attribute_name);

  if (QUILL_UNLIKELY(search == attr_map.cend()))
  {
    std::ostringstream error_msg;
    error_msg << "Attribute enum value does not exist for attribute_name "
              << "\"" << attribute_name << "\"";
    QUILL_THROW(QuillError{error_msg.str()});
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

  std::array<size_t, PatternFormatter::Attribute::ATTR_NR_ITEMS> order_index{};
  order_index.fill(PatternFormatter::Attribute::ATTR_NR_ITEMS - 1);

  std::array<fmtquill::detail::named_arg_info<char>, PatternFormatter::Attribute::ATTR_NR_ITEMS> named_args{};
  _store_named_args<0, 0>(named_args, args...);
  uint8_t arg_idx = 0;

  // we will replace all %(....) with {} to construct a string to pass to fmt library
  std::size_t arg_identifier_pos = pattern.find_first_of('%');
  while (arg_identifier_pos != std::string::npos)
  {
    if (std::size_t const open_paren_pos = pattern.find_first_of('(', arg_identifier_pos);
        open_paren_pos != std::string::npos && (open_paren_pos - arg_identifier_pos) == 1)
    {
      // if we found '%(' we have a matching pattern and we now need to get the closed paren
      std::size_t const closed_paren_pos = pattern.find_first_of(')', open_paren_pos);

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
      PatternFormatter::Attribute const attr_enum_value = attribute_from_string(attr_name);
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

} // namespace

/***/
void PatternFormatter::_set_pattern(std::string format_pattern)
{
  format_pattern += "\n";

  // the order we pass the arguments here must match with the order of Attribute enum
  using namespace fmtquill::literals;
  std::tie(_format, _order_index) = _generate_fmt_format_string(
    _is_set_in_pattern, std::string{format_pattern}, "time"_a = "", "file_name"_a = "",
    "caller_function"_a = "", "log_level"_a = "", "log_level_id"_a = "", "line_number"_a = "",
    "logger"_a = "", "full_path"_a = "", "thread_id"_a = "", "thread_name"_a = "",
    "process_id"_a = "", "source_location"_a = "", "short_source_location"_a = "", "message"_a = "",
    "custom_tags"_a = "", "structured_keys"_a = "");

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
  _set_arg<Attribute::CustomTags>(std::string_view("custom_tags"));
  _set_arg<Attribute::StructuredKeys>(std::string_view("structured_keys"));
}

/***/
fmt_buffer_t const& PatternFormatter::format(
  std::chrono::nanoseconds timestamp, std::string_view thread_id, std::string_view thread_name,
  std::string_view process_id, std::string_view logger, std::string_view log_level,
  MacroMetadata const& log_statement_metadata,
  std::vector<std::pair<std::string, transit_event_fmt_buffer_t>> const& structured_kvs,
  transit_event_fmt_buffer_t const& log_msg)
{
  // clear out existing buffer
  _formatted_log_message.clear();

  if (_format.empty())
  {
    // nothing to format when the given format is empty. This is useful e.g. in the JsonFileHandler
    // if we want to skip formatting the main message
    return _formatted_log_message;
  }

  if (_is_set_in_pattern[Attribute::Time])
  {
    _set_arg_val<Attribute::Time>(_timestamp_formatter.format_timestamp(timestamp));
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
    _structured_keys.clear();
    for (size_t i = 0; i < structured_kvs.size(); ++i)
    {
      _structured_keys += structured_kvs[i].first;

      if (i != structured_kvs.size() - 1)
      {
        _structured_keys += ", ";
      }
    }

    _set_arg_val<Attribute::StructuredKeys>(_structured_keys);
  }

  if (_is_set_in_pattern[Attribute::CustomTags])
  {
    if (log_statement_metadata.custom_tags())
    {
      _custom_tags.clear();
      log_statement_metadata.custom_tags()->format(_custom_tags);
      _set_arg_val<Attribute::CustomTags>(_custom_tags);
    }
    else
    {
      _set_arg_val<Attribute::CustomTags>(std::string{});
    }
  }

  _set_arg_val<Attribute::Message>(std::string_view{log_msg.begin(), log_msg.size()});

  fmtquill::vformat_to(std::back_inserter(_formatted_log_message), _format,
                       fmtquill::basic_format_args(_args.data(), static_cast<int>(_args.size())));

  return _formatted_log_message;
}

/***/
std::string_view PatternFormatter::format_timestamp(std::chrono::nanoseconds timestamp)
{
  return _timestamp_formatter.format_timestamp(timestamp);
}
} // namespace quill