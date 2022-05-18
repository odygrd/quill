#include "quill/PatternFormatter.h"
#include <algorithm> // for max

namespace quill
{

namespace
{
/***/
template <size_t, size_t>
constexpr void _store_named_args(std::array<fmt::detail::named_arg_info<char>, PatternFormatter::Attribute::ATTR_NR_ITEMS>&)
{
}

/***/
template <size_t Idx, size_t NamedIdx, typename Arg, typename... Args>
constexpr void _store_named_args(std::array<fmt::detail::named_arg_info<char>, PatternFormatter::Attribute::ATTR_NR_ITEMS>& named_args_store,
                                 const Arg& arg, const Args&... args)
{
  named_args_store[NamedIdx] = {arg.name, Idx};
  _store_named_args<Idx + 1, NamedIdx + 1>(named_args_store, args...);
}

/**
 * Convert the pattern to fmt format string and also populate the order index array
 * e.g. given :
 *   "%(ascii_time) [%(thread)] %(filename):%(lineno) %(level_name:<12) %(logger_name) - "
 *
 * is changed to :
 *  {} [{}] {}:{} {:<12} {} -
 *
 *  with a order index of :
 *  i: 0 order idx[i] is: 0 - %(ascii_time)
 *  i: 1 order idx[i] is: 2 - %(filename)
 *  i: 2 order idx[i] is: 10 - empty
 *  i: 3 order idx[i] is: 4 - %(level_name)
 *  i: 4 order idx[i] is: 10 - empty
 *  i: 5 order idx[i] is: 3 - %(lineno)
 *  i: 6 order idx[i] is: 5 - %(logger_name)
 *  i: 7 order idx[i] is: 10 - empty
 *  i: 8 order idx[i] is: 1 - %(thread)
 *  i: 9 order idx[i] is: 10 - empty
 *  i: 10 order idx[i] is: 10 - empty
 * @tparam Args
 * @param pattern
 * @param args
 * @return
 */
template <typename... Args>
QUILL_NODISCARD std::pair<std::string, std::array<size_t, PatternFormatter::Attribute::ATTR_NR_ITEMS>> _generate_fmt_format_string(
  std::string pattern, Args const&... args)
{
  // Attribute enum and the args we are passing here must be in sync
  static_assert(PatternFormatter::Attribute::ATTR_NR_ITEMS == sizeof...(Args));

  std::array<size_t, PatternFormatter::Attribute::ATTR_NR_ITEMS> order_index{};
  order_index.fill(PatternFormatter::Attribute::ATTR_NR_ITEMS - 1);

  std::array<fmt::detail::named_arg_info<char>, PatternFormatter::Attribute::ATTR_NR_ITEMS> named_args{};
  _store_named_args<0, 0>(named_args, args...);
  uint8_t arg_idx = 0;

  // we will replace all %(....) with {} to construct a string to pass to fmt library
  std::size_t arg_identifier_pos = pattern.find_first_of('%');
  while (arg_identifier_pos != std::string::npos)
  {
    std::size_t open_paren_pos = pattern.find_first_of('(', arg_identifier_pos);
    if (open_paren_pos != std::string::npos && (open_paren_pos - arg_identifier_pos) == 1)
    {
      // if we found '%(' we have a matching pattern and we now need to get the closed paren
      std::size_t closed_paren_pos = pattern.find_first_of(')', open_paren_pos);

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
        // e.g. %(fileline:<32)

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
        // e.g. fileline
        attr_name = attr.substr(2, pos - 2);
      }
      else
      {
        // Make the replacement.
        pattern.replace(arg_identifier_pos, attr.length(), "{}");

        // Get the part that is the named argument
        // e.g. fileline
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
        QUILL_THROW(QuillError{"Invalid format pattern"});
      }

      order_index[static_cast<size_t>(id)] = arg_idx++;

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
void PatternFormatter::_set_pattern(std::string const& format_pattern)
{
  // parse and check the given pattern
  constexpr std::string_view message{"%(message)"};
  size_t const message_found = format_pattern.find(message);

  if (message_found == std::string::npos)
  {
    QUILL_THROW(QuillError{"%(message) is required in the format pattern"});
  }

  // the order we pass the arguments here must match with the order of Attribute enum
  using namespace fmt::literals;
  std::tie(_format, _order_index) = _generate_fmt_format_string(
    std::string{format_pattern}, "ascii_time"_a = "", "filename"_a = "", "function_name"_a = "",
    "level_name"_a = "", "level_id"_a = "", "lineno"_a = "", "logger_name"_a = "", "pathname"_a = "",
    "thread"_a = "", "thread_name"_a = "", "process"_a = "", "fileline"_a = "", "message"_a = "");

  _set_arg<Attribute::AsciiTime>(std::string_view("ascii_time"));
  _set_arg<Attribute::FileName>(std::string_view("filename"));
  _set_arg<Attribute::FunctionName>(std::string_view("function_name"));
  _set_arg<Attribute::LevelName>(std::string_view("level_name"));
  _set_arg<Attribute::LevelId>(std::string_view("level_id"));
  _set_arg<Attribute::LineNo>(std::string_view("lineno"));
  _set_arg<Attribute::LoggerName>(std::string_view("logger_name"));
  _set_arg<Attribute::PathName>(std::string_view("pathname"));
  _set_arg<Attribute::Thread>(std::string_view("thread"));
  _set_arg<Attribute::ThreadName>(std::string_view("thread_name"));
  _set_arg<Attribute::Process>(std::string_view("process"));
  _set_arg<Attribute::FileLine>(std::string_view("fileline"));
  _set_arg<Attribute::Message>(std::string_view("message"));
}

/***/
void PatternFormatter::format(std::chrono::nanoseconds timestamp, std::string_view thread_id,
                              std::string_view thread_name, std::string_view process_id, std::string_view logger_name,
                              MacroMetadata const& macro_metadata, fmt::memory_buffer const& log_msg)
{
  // clear out existing buffer
  _formatted_log_message.clear();

  std::string const fileline = fmt::format("{}:{}", macro_metadata.filename(), macro_metadata.lineno());

  _set_arg_val<Attribute::AsciiTime>(_timestamp_formatter.format_timestamp(timestamp));
  _set_arg_val<Attribute::FileName>(macro_metadata.filename());
  _set_arg_val<Attribute::FunctionName>(macro_metadata.func());
  _set_arg_val<Attribute::LevelName>(macro_metadata.level_as_str());
  _set_arg_val<Attribute::LevelId>(macro_metadata.level_id_as_str());
  _set_arg_val<Attribute::LineNo>(macro_metadata.lineno());
  _set_arg_val<Attribute::LoggerName>(logger_name);
  _set_arg_val<Attribute::PathName>(macro_metadata.pathname());
  _set_arg_val<Attribute::Thread>(thread_id);
  _set_arg_val<Attribute::ThreadName>(thread_name);
  _set_arg_val<Attribute::Process>(process_id);
  _set_arg_val<Attribute::FileLine>(std::string_view{fileline});
  _set_arg_val<Attribute::Message>(std::string_view{log_msg.begin(), log_msg.size()});

  fmt::vformat_to(std::back_inserter(_formatted_log_message), _format,
                  fmt::basic_format_args(_args.data(), static_cast<int>(_args.size())));

  // Append a new line
  _formatted_log_message.push_back('\n');
}

} // namespace quill