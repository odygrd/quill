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
  // We do not care about all those if/else's as they are used only during init to store the correct
  // callback to our tuple
  if (pattern_attr == "ascii_time")
  {
    return [this](std::chrono::time_point<std::chrono::system_clock> timestamp, uint32_t,
                  char const*, detail::StaticLogRecordInfo const&) {
      // TODO pass the date format string
      _convert_epoch_to_local_date(timestamp);
      return _formatted_date.data();
    };
  }
  else if (pattern_attr == "thread")
  {
    return [this](std::chrono::time_point<std::chrono::system_clock>, uint32_t thread_id, char const*,
              detail::StaticLogRecordInfo const&) { _thread_id = fmt::to_string(thread_id); return _thread_id.data(); };
  }
  else if (pattern_attr == "pathname")
  {
    return [](std::chrono::time_point<std::chrono::system_clock>, uint32_t, char const*,
              detail::StaticLogRecordInfo const& logline_info) { return logline_info.pathname(); };
  }
  else if (pattern_attr == "filename")
  {
    return [](std::chrono::time_point<std::chrono::system_clock>, uint32_t, char const*,
              detail::StaticLogRecordInfo const& logline_info) { return logline_info.filename(); };
  }
  else if (pattern_attr == "lineno")
  {
    return [](std::chrono::time_point<std::chrono::system_clock>, uint32_t, char const*,
              detail::StaticLogRecordInfo const& logline_info) { return logline_info.lineno(); };
  }
  else if (pattern_attr == "level_name")
  {
    return
      [](std::chrono::time_point<std::chrono::system_clock>, uint32_t, char const*,
         detail::StaticLogRecordInfo const& logline_info) { return logline_info.level_as_str(); };
  }
  else if (pattern_attr == "logger_name")
  {
    return [](std::chrono::time_point<std::chrono::system_clock>, uint32_t, char const* logger_name,
              detail::StaticLogRecordInfo const&) { return logger_name; };
  }
  else if (pattern_attr == "function_name")
  {
    return [](std::chrono::time_point<std::chrono::system_clock>, uint32_t, char const*,
              detail::StaticLogRecordInfo const& logline_info) { return logline_info.func(); };
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

  void PatternFormatter::_convert_epoch_to_local_date(std::chrono::system_clock::time_point epoch_time,
                                                         char const* date_format /* = "%H:%M:%S" */)
  {
    int64_t const epoch = epoch_time.time_since_epoch().count();

    // convert timestamp to date
    int64_t const rawtime_seconds = epoch / 1'000'000'000;

    tm timeinfo;
    localtime_r(&rawtime_seconds, std::addressof(timeinfo));

    // extract the nanoseconds
    std::uint32_t const usec = epoch - (rawtime_seconds * 1'000'000'000);

    _formatted_date.clear();

    // add time
    auto res = strftime(&_formatted_date[0], _formatted_date.capacity(), date_format, std::addressof(timeinfo));

    while (QUILL_UNLIKELY(res == 0))
    {
      _formatted_date.resize(_formatted_date.capacity() * 2);
      res = strftime(&_formatted_date[0], _formatted_date.capacity(), date_format, std::addressof(timeinfo));
    }

    if (QUILL_UNLIKELY(res + 12 > _formatted_date.capacity()))
    {
      _formatted_date.resize(res + 12);
    }

    // TODO: Fix/check the format string the user can pass for date and provide option for us ?
    // add the nanoseconds using the fast integer formatter from fmt
    _formatted_date[res] = ('.');
    fmt::format_int i(usec);
    memcpy(&_formatted_date[res + 1], i.data(), i.size());
  }
  } // namespace quill