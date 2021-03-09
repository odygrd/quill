#include "quill/detail/misc/Utilities.h"

#include "quill/detail/misc/Os.h" // for localtime_rs, ..
#include <chrono>
#include <codecvt> // for codecvt_utf8
#include <cstring>
#include <ctime>
#include <locale> // for wstring_convert
#include <sstream>

namespace quill
{
namespace detail
{
/***/
std::wstring s2ws(std::string const& str) noexcept
{
#if defined(__MINGW32__) || defined(__MINGW64__)
  using convert_t = std::codecvt_utf8_utf16<wchar_t>;
#else
  using convert_t = std::codecvt_utf8<wchar_t>;
#endif
  std::wstring_convert<convert_t, wchar_t> converter;

  return converter.from_bytes(str);
}

/***/
std::string ws2s(std::wstring const& wstr) noexcept
{
#if defined(__MINGW32__) || defined(__MINGW64__)
  using convert_t = std::codecvt_utf8_utf16<wchar_t>;
#else
  using convert_t = std::codecvt_utf8<wchar_t>;
#endif
  std::wstring_convert<convert_t, wchar_t> converter;

  return converter.to_bytes(wstr);
}

/***/
void replace_all(std::string& str, std::string const& old_value, std::string const& new_value) noexcept
{
  std::string::size_type pos = 0u;
  while ((pos = str.find(old_value, pos)) != std::string::npos)
  {
    str.replace(pos, old_value.length(), new_value);
    pos += new_value.length();
  }
}

/***/
time_t nearest_hour_timestamp(time_t timestamp) noexcept
{
  time_t const nearest_hour_ts = timestamp - (timestamp % 3600);
  return nearest_hour_ts;
}

/***/
time_t next_hour_timestamp(time_t timestamp) noexcept
{
  time_t const next_hour_ts = nearest_hour_timestamp(timestamp) + 3600;
  return next_hour_ts;
}

/***/
time_t next_noon_or_midnight_timestamp(time_t timestamp, Timezone timezone) noexcept
{
  // Get the current date and time now as time_info
  tm time_info;

  if (timezone == Timezone::GmtTime)
  {
    detail::gmtime_rs(&timestamp, &time_info);
  }
  else
  {
    detail::localtime_rs(&timestamp, &time_info);
  }

  if (time_info.tm_hour < 12)
  {
    // we are before noon, so calculate noon
    time_info.tm_hour = 11;
    time_info.tm_min = 59;
    time_info.tm_sec = 59; // we add 1 second later
  }
  else
  {
    // we are after noon so we calculate midnight
    time_info.tm_hour = 23;
    time_info.tm_min = 59;
    time_info.tm_sec = 59; // we add 1 second later
  }

  // convert back to time since epoch
  std::chrono::system_clock::time_point const next_midnight = (timezone == Timezone::GmtTime)
    ? std::chrono::system_clock::from_time_t(quill::detail::timegm(&time_info))
    : std::chrono::system_clock::from_time_t(std::mktime(&time_info));

  // returns seconds since epoch of the next midnight.
  return std::chrono::duration_cast<std::chrono::seconds>(next_midnight.time_since_epoch()).count() + 1;
}

/***/
std::vector<char> safe_strftime(char const* format_string, time_t timestamp, Timezone timezone)
{
  if (strlen(format_string) == 0)
  {
    std::vector<char> res;
    res.push_back('\0');
    return res;
  }

  // Convert timestamp to time_info
  tm time_info;
  if (timezone == Timezone::LocalTime)
  {
    detail::localtime_rs(reinterpret_cast<time_t const*>(std::addressof(timestamp)), std::addressof(time_info));
  }
  else if (timezone == Timezone::GmtTime)
  {
    detail::gmtime_rs(reinterpret_cast<time_t const*>(std::addressof(timestamp)), std::addressof(time_info));
  }

  // Create a buffer to call strftimex
  std::vector<char> buffer;
  buffer.resize(32);
  size_t res = strftime(&buffer[0], buffer.size(), format_string, std::addressof(time_info));

  while (res == 0)
  {
    // if strftime fails we will reserve more space
    buffer.resize(buffer.size() * 2);
    res = strftime(&buffer[0], buffer.size(), format_string, std::addressof(time_info));
  }

  return buffer;
}

/***/
std::vector<std::string> split(std::string const& s, char delimiter)
{
  std::vector<std::string> tokens;
  std::istringstream token_stream(s);

  std::string token;
  while (std::getline(token_stream, token, delimiter))
  {
    if (!token.empty())
    {
      tokens.push_back(token);
    }
  }
  return tokens;
}

} // namespace detail
} // namespace quill
