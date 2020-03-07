#include "quill/PatternFormatter.h"
#include "quill/detail/misc/Macros.h"
#include "quill/detail/misc/Os.h"

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
    return [this](std::chrono::nanoseconds timestamp, char const*, char const*,
                  detail::StaticLogRecordInfo const&) {
      // TODO pass the date format string for date formatting
      _convert_epoch_to_local_date(timestamp);
      return _formatted_date.data();
    };
  }
  else if (pattern_attr == "thread")
  {
    return [](std::chrono::nanoseconds, char const* thread_id, char const*,
              detail::StaticLogRecordInfo const&) { return thread_id; };
  }
  else if (pattern_attr == "pathname")
  {
    return [](std::chrono::nanoseconds, char const*, char const*,
              detail::StaticLogRecordInfo const& logline_info) { return logline_info.pathname(); };
  }
  else if (pattern_attr == "filename")
  {
    return [](std::chrono::nanoseconds, char const*, char const*,
              detail::StaticLogRecordInfo const& logline_info) { return logline_info.filename(); };
  }
  else if (pattern_attr == "lineno")
  {
    return [](std::chrono::nanoseconds, char const*, char const*,
              detail::StaticLogRecordInfo const& logline_info) { return logline_info.lineno(); };
  }
  else if (pattern_attr == "level_name")
  {
    return
      [](std::chrono::nanoseconds, char const*, char const*,
         detail::StaticLogRecordInfo const& logline_info) { return logline_info.level_as_str(); };
  }
  else if (pattern_attr == "logger_name")
  {
    return [](std::chrono::nanoseconds, char const*, char const* logger_name,
              detail::StaticLogRecordInfo const&) { return logger_name; };
  }
  else if (pattern_attr == "function_name")
  {
    return [](std::chrono::nanoseconds, char const*, char const*,
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

void PatternFormatter::_convert_epoch_to_local_date(std::chrono::nanoseconds epoch_time)
{
  int64_t const epoch = epoch_time.count();

  // convert timestamp to date
  int64_t const rawtime_seconds = epoch / 1'000'000'000;

  tm timeinfo;

  // Convert timestamp to date based on the option
  if (_timezone_type == Timezone::GmtTime)
  {
    detail::gmtime_rs(reinterpret_cast<time_t const*>(std::addressof(rawtime_seconds)), std::addressof(timeinfo));
  }
  else if (_timezone_type == Timezone::LocalTime)
  {
    detail::localtime_rs(reinterpret_cast<time_t const*>(std::addressof(rawtime_seconds)),
                         std::addressof(timeinfo));
  }
  else
  {
    throw std::runtime_error("Unknown timezone type");
  }

  // extract the nanoseconds
  auto const usec = static_cast<uint64_t>(epoch - (rawtime_seconds * 1'000'000'000));

  _formatted_date.clear();

  // add time
  auto res = strftime(&_formatted_date[0], _formatted_date.capacity(), _date_format.data(),
                      std::addressof(timeinfo));

  while (QUILL_UNLIKELY(res == 0))
  {
    _formatted_date.resize(_formatted_date.capacity() * 2);
    res = strftime(&_formatted_date[0], _formatted_date.capacity(), _date_format.data(),
                   std::addressof(timeinfo));
  }

  // Add precision
  if (_timestamp_precision == TimestampPrecision::NanoSeconds)
  {
    if (QUILL_UNLIKELY(res + 11 > _formatted_date.capacity()))
    {
      _formatted_date.resize(res + 11);
    }

    // add the nanoseconds using the fast integer formatter from fmt
    _formatted_date[res] = ('.');
    size_t usec_begin = res + 1;
    size_t const usec_end = usec_begin + 9;

    // Fill with zeros the nanosecond precision
    memset(&_formatted_date[usec_begin], '0', 9);
    fmt::format_int i(usec);

    usec_begin = usec_end - i.size();
    memcpy(&_formatted_date[usec_begin], i.data(), i.size());
    _formatted_date[usec_end] = '\0';
  }
  else if (_timestamp_precision == TimestampPrecision::MicroSeconds)
  {
    if (QUILL_UNLIKELY(res + 8 > _formatted_date.capacity()))
    {
      _formatted_date.resize(res + 8);
    }

    // add the microseconds using the fast integer formatter from fmt
    _formatted_date[res] = ('.');
    size_t usec_begin = res + 1;
    size_t const usec_end = usec_begin + 6;

    // Fill with zeros the nanosecond precision
    memset(&_formatted_date[usec_begin], '0', 6);
    fmt::format_int i(usec / 1000);

    usec_begin = usec_end - i.size();
    memcpy(&_formatted_date[usec_begin], i.data(), i.size());
    _formatted_date[usec_end] = '\0';
  }
  else if (_timestamp_precision == TimestampPrecision::MilliSeconds)
  {
    if (QUILL_UNLIKELY(res + 5 > _formatted_date.capacity()))
    {
      _formatted_date.resize(res + 5);
    }

    // add the microseconds using the fast integer formatter from fmt
    _formatted_date[res] = ('.');
    size_t usec_begin = res + 1;
    size_t const usec_end = usec_begin + 3;

    // Fill with zeros the nanosecond precision
    memset(&_formatted_date[usec_begin], '0', 3);
    fmt::format_int i(usec / 1'000'000);

    usec_begin = usec_end - i.size();
    memcpy(&_formatted_date[usec_begin], i.data(), i.size());
    _formatted_date[usec_end] = '\0';
  }
}
} // namespace quill