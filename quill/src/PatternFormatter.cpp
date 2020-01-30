#include "quill/PatternFormatter.h"

namespace quill
{

/***/
PatternFormatter::PatternFormatter(PatternFormatter const& other)
  : _pattern_formatter_helper_part_1(
      other._pattern_formatter_helper_part_1 ? other._pattern_formatter_helper_part_1->clone() : nullptr),
    _pattern_formatter_helper_part_3(
      other._pattern_formatter_helper_part_3 ? other._pattern_formatter_helper_part_3->clone() : nullptr)
{
}

/***/
PatternFormatter::PatternFormatter(PatternFormatter&& other) noexcept
  : _pattern_formatter_helper_part_1(std::move(other._pattern_formatter_helper_part_1)),
    _pattern_formatter_helper_part_3(std::move(other._pattern_formatter_helper_part_3))
{
}

/***/
PatternFormatter& PatternFormatter::operator=(PatternFormatter const& other)
{
  if (this != &other)
  {
    _pattern_formatter_helper_part_1.reset(
      other._pattern_formatter_helper_part_1 ? other._pattern_formatter_helper_part_1->clone() : nullptr);
    _pattern_formatter_helper_part_3.reset(
      other._pattern_formatter_helper_part_3 ? other._pattern_formatter_helper_part_3->clone() : nullptr);
  }
  return *this;
}

/***/
PatternFormatter& PatternFormatter::operator=(PatternFormatter&& other) noexcept
{
  if (this != &other)
  {
    _pattern_formatter_helper_part_1 = std::move(other._pattern_formatter_helper_part_1);
    _pattern_formatter_helper_part_3 = std::move(other._pattern_formatter_helper_part_3);
  }
  return *this;
}

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
        throw std::runtime_error("Incorrect pattern. Missing ')'");
      }

      // We have everything, get the substring
      std::string const pattern_attr =
        pattern.substr(open_paren_pos + 1, closed_paren_pos - open_paren_pos - 1);

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
  if (pattern_attr == "ascii_time")
  {
    return [](uint64_t timestamp, uint32_t, char const*, detail::StaticLogRecordInfo const&) {
      // TODO : formatting to string ts
      return std::to_string(timestamp);
    };
  }
  else if (pattern_attr == "thread")
  {
    return [](uint64_t, uint32_t thread_id, char const*, detail::StaticLogRecordInfo const&) {
      return std::to_string(thread_id);
    };
  }
  else if (pattern_attr == "pathname")
  {
    return [](uint64_t, uint32_t, char const*, detail::StaticLogRecordInfo const& logline_info) {
      return logline_info.pathname();
    };
  }
  else if (pattern_attr == "filename")
  {
    return [](uint64_t, uint32_t, char const*, detail::StaticLogRecordInfo const& logline_info) {
      return logline_info.filename();
    };
  }
  else if (pattern_attr == "lineno")
  {
    return [](uint64_t, uint32_t, char const*, detail::StaticLogRecordInfo const& logline_info) {
      return std::to_string(logline_info.lineno());
    };
  }
  else if (pattern_attr == "level_name")
  {
    return [](uint64_t, uint32_t, char const*, detail::StaticLogRecordInfo const& logline_info) {
      return logline_info.level_as_str();
    };
  }
  else if (pattern_attr == "logger_name")
  {
    return [](uint64_t, uint32_t, char const* logger_name, detail::StaticLogRecordInfo const&) {
      return logger_name;
    };
  }
  else if (pattern_attr == "function_name")
  {
    return [](uint64_t, uint32_t, char const*, detail::StaticLogRecordInfo const& logline_info) {
      return logline_info.func();
    };
  }
  else
  {
    throw std::runtime_error("Invalid attribute name for logger pattern");
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
        throw std::runtime_error("Incorrect pattern. Missing ')'");
      }

      // We have everything, get the substring, this time including '%( )'
      std::string const pattern_attr =
        pattern.substr(arg_identifier_pos, (closed_paren_pos + 1) - arg_identifier_pos);

      // Make the replacement.
      pattern.replace(arg_identifier_pos, pattern_attr.length(), "{}");

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