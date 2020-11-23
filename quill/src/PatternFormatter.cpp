#include "quill/PatternFormatter.h"
#include "quill/detail/LogManager.h"          // for LogManager
#include "quill/detail/LogManagerSingleton.h" // for LogManagerSingleton
#include <algorithm>                          // for max

namespace quill
{
/***/
std::vector<PatternFormatter::argument_callback_t> PatternFormatter::_generate_vector_of_callbacks(std::string const& pattern)
{
  std::vector<argument_callback_t> callback_collection;

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
        QUILL_THROW(QuillError{"Incorrect pattern. Missing ')'"});
      }

      // We have everything, get the substring
      std::string pattern_attr = pattern.substr(open_paren_pos + 1, closed_paren_pos - open_paren_pos - 1);

      // find any user format specifiers
      size_t const pos = pattern_attr.find(':');

      if (pos != std::string::npos)
      {
        // we found user format specifiers that we need to remove before we pass the pattern_attr
        pattern_attr = pattern_attr.substr(0, pos);
      }

      // Add the function to the vector
      callback_collection.emplace_back(_select_argument_callback(pattern_attr));

      // search for the next '%'
      arg_identifier_pos = pattern.find_first_of('%', closed_paren_pos);
    }
    else
    {
      // search for the next '%'
      arg_identifier_pos = pattern.find_first_of('%', arg_identifier_pos + 1);
    }
  }
  return callback_collection;
}

/***/
PatternFormatter::argument_callback_t PatternFormatter::_select_argument_callback(std::string const& pattern_attr)
{
  // We do not care about the performance of all those if/else's as they are only used during
  // initialisation to store the correct callback to our tuple

  if (pattern_attr == "ascii_time")
  {
    return [this](std::chrono::nanoseconds timestamp, char const*, char const*, LogMacroMetadata const&) {
      return _timestamp_formatter.format_timestamp(timestamp);
    };
  }
  else if (pattern_attr == "thread")
  {
    return [](std::chrono::nanoseconds, char const* thread_id, char const*,
              LogMacroMetadata const&) { return thread_id; };
  }
  else if (pattern_attr == "process")
  {
    return [](std::chrono::nanoseconds, char const*, char const*, LogMacroMetadata const&) {
      return detail::LogManagerSingleton::instance().log_manager().process_id().data();
    };
  }
  else if (pattern_attr == "pathname")
  {
    return [](std::chrono::nanoseconds, char const*, char const*,
              LogMacroMetadata const& logline_info) { return logline_info.pathname(); };
  }
  else if (pattern_attr == "filename")
  {
    return [](std::chrono::nanoseconds, char const*, char const*,
              LogMacroMetadata const& logline_info) { return logline_info.filename(); };
  }
  else if (pattern_attr == "lineno")
  {
    return [](std::chrono::nanoseconds, char const*, char const*,
              LogMacroMetadata const& logline_info) { return logline_info.lineno(); };
  }
  else if (pattern_attr == "fileline")
  {
    return [this](std::chrono::nanoseconds, char const*, char const*, LogMacroMetadata const& logline_info) {
      _fileline.clear();
      _fileline += logline_info.filename();
      _fileline += ":";
      _fileline += logline_info.lineno();
      return _fileline.data();
    };
  }
  else if (pattern_attr == "level_name")
  {
    return [](std::chrono::nanoseconds, char const*, char const*,
              LogMacroMetadata const& logline_info) { return logline_info.level_as_str(); };
  }
  else if (pattern_attr == "logger_name")
  {
    return [](std::chrono::nanoseconds, char const*, char const* logger_name,
              LogMacroMetadata const&) { return logger_name; };
  }
  else if (pattern_attr == "function_name")
  {
#if defined(_WIN32)
    return [this](std::chrono::nanoseconds, char const*, char const*, LogMacroMetadata const& logline_info) {
      // On windows, __FUNCTION__ also contains the namespace name e.g. a::b::my_function().
      _win_func = logline_info.func();

      // replace all "::" with '.'
      size_t index = _win_func.find("::");
      while (index != std::string::npos)
      {
        _win_func.replace(index, 2, ".");
        index = _win_func.find("::");
      }

      return _win_func.data();
    };
#else
    return [](std::chrono::nanoseconds, char const*, char const*,
              LogMacroMetadata const& logline_info) { return logline_info.func(); };
#endif
  }
  else
  {
    QUILL_THROW(QuillError{"Invalid attribute name for logger pattern"});
  }
}

/***/
std::string PatternFormatter::_generate_fmt_format_string(std::string pattern)
{
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
        QUILL_THROW(QuillError{"Incorrect pattern. Missing ')'"});
      }

      // We have everything, get the substring, this time including '%( )'
      std::string const pattern_attr =
        pattern.substr(arg_identifier_pos, (closed_paren_pos + 1) - arg_identifier_pos);

      // find any user format specifiers
      size_t const pos = pattern_attr.find(':');

      if (pos != std::string::npos)
      {
        // we found user format specifiers that we want to keep.
        // e.g. %(fileline:<32)
        std::string custom_format_specifier = pattern_attr.substr(pos);
        custom_format_specifier.pop_back(); // remove ")"

        // replace with the pattern with the correct value
        std::string value;
        value += "{";
        value += custom_format_specifier;
        value += "}";

        // e.g. {:<32}
        pattern.replace(arg_identifier_pos, pattern_attr.length(), value);
      }
      else
      {
        // Make the replacement.
        pattern.replace(arg_identifier_pos, pattern_attr.length(), "{}");
      }

      // Look for the next pattern to replace
      arg_identifier_pos = pattern.find_first_of('%');
    }
    else
    {
      // search for the next '%'
      arg_identifier_pos = pattern.find_first_of('%', arg_identifier_pos + 1);
    }
  }
  return pattern;
}

} // namespace quill