#include "quill/detail/backend/StringFromTime.h"
#include "quill/QuillError.h"
#include "quill/detail/misc/Os.h"
#include "quill/detail/misc/Utilities.h"
#include <array>
#include <chrono>
#include <map>

namespace
{

/**
 * Splits the given timestamp format into three parts on the first modifier
 * Part 1 is the string before the modifier
 * Part 2 is the modifier itself
 * Part 3 is the timestamp_format after the modifier
 * @param timestamp_format the given timestamp format to split. The string is modified
 * @return A pair consisting of part 1 and part 2 if modifier is found. If no modifier found then part 1 and part 2 are empty
 */
std::pair<std::string, std::string> _split_timestamp_format_once(std::string& timestamp_format) noexcept
{
  static std::array<std::string, 7> const modifiers{"%H", "%M", "%S", "%I", "%k", "%l", "%s"};

  // if we find any modifier in the timestamp format we store the index where we
  // found it.
  // We use a map to find the first modifier (with the lowest index) in the given string
  // Maps found_index -> modifier value
  std::map<size_t, std::string> found_format_modifiers;

  for (auto const& modifier : modifiers)
  {
    auto const search = timestamp_format.find(modifier);
    if (search != std::string::npos)
    {
      // Add the index and the modifier string to our map
      found_format_modifiers.insert(std::make_pair(search, modifier));
    }
  }

  if (found_format_modifiers.empty())
  {
    // We didn't find any modifiers in the given string, therefore we return
    // both parts as empty
    return std::make_pair(std::string{}, std::string{});
  }

  // we will split the formatted timestamp on the first modifier we found

  // Part 1 is the part before the modifier
  // Here we check that there is actually a part of before and the format doesn't start with the
  // modifier, otherwise we use an empty string
  std::string const part_1 = found_format_modifiers.begin()->first > 0
    ? std::string{timestamp_format.data(), found_format_modifiers.begin()->first}
    : "";

  // The actual value of the modifier string
  std::string const part_2 = found_format_modifiers.begin()->second;

  // We modify the given timestamp string to exclude part_1 and part_2.
  // part_2 length as the modifier value will always be 2
  timestamp_format = std::string{timestamp_format.data() + found_format_modifiers.begin()->first + 2};

  return std::make_pair(part_1, part_2);
}
} // namespace

namespace quill
{
namespace detail
{

/***/
void StringFromTime::init(std::string timestamp_format, Timezone timezone)
{
  _timestamp_format = std::move(timestamp_format);
  _time_zone = timezone;

  if (_timestamp_format.find("%X") != std::string::npos)
  {
    QUILL_THROW(QuillError("`%X` as format modifier is not currently supported in format: " + _timestamp_format));
  }

  // We first look for some special format modifiers and replace them
  replace_all(_timestamp_format, "%r", "%I:%M:%S %p");
  replace_all(_timestamp_format, "%R", "%H:%M");
  replace_all(_timestamp_format, "%T", "%H:%M:%S");

  // Populate the initial parts that we will use to generate a pre-formatted string
  _populate_initial_parts(_timestamp_format);

  // Get the current timestamp now
  time_t timestamp;
  std::time(&timestamp);

  if (_time_zone == Timezone::LocalTime)
  {
    // If localtime is used we will recalculate every 1 hour - this is because of the DST changes
    // and an easy work-around

    // Then round it down to the nearest hour
    timestamp = nearest_hour_timestamp(timestamp);

    // Also calculate and store the next hour timestamp
    _next_recalculation_timestamp = next_hour_timestamp(timestamp);
  }
  else if (_time_zone == Timezone::GmtTime)
  {
    // otherwise we will only recalculate every noon and midnight. the reason for this is in case
    // user is using PM, AM format etc
    _next_recalculation_timestamp = next_noon_or_midnight_timestamp(timestamp, _time_zone);

    // we don't need to modify timestamp in the case of UTC
  }

  // Now populate a pre formatted string for this hour,
  // also cache any indexes of the time modifier in the string
  _populate_pre_formatted_string_and_cached_indexes(timestamp);
}

/***/
std::string const& StringFromTime::format_timestamp(time_t timestamp)
{
  // First we check for the edge case where the given timestamp is back in time. This is when
  // the timestamp provided is less than our cached_timestamp. We only expect to format timestamps
  // that are incrementing not those back in time. In this case we just fall back to calling strfime
  if (timestamp < _cached_timestamp)
  {
    _fallback_formatted = safe_strftime(_timestamp_format.data(), timestamp, _time_zone).data();
    return _fallback_formatted;
  }

  // After this point we know that given timestamp is >= to the cache timestamp.

  // We check if the given timestamp greater than the _next_recalculation_timestamp to recalculate
  if (timestamp >= _next_recalculation_timestamp)
  {
    // in this case we have to populate our cached string again using strftime
    _pre_formatted_ts.clear();
    _cached_indexes.clear();

    // Now populate a pre formatted string for the next rec
    _populate_pre_formatted_string_and_cached_indexes(timestamp);

    if (_time_zone == Timezone::LocalTime)
    {
      // Update the timestamp to point to the next hour, here we can just add 3600 as the _next_hour_timestamp was already rounded to point to sharp minutes before
      _next_recalculation_timestamp = timestamp + 3600;
    }
    else if (_time_zone == Timezone::GmtTime)
    {
      _next_recalculation_timestamp =
        next_noon_or_midnight_timestamp(timestamp + 1, _time_zone);
    }
  }

  if (_cached_indexes.empty())
  {
    // if we don't have to format any hours minutes or seconds we can just return here
    return _pre_formatted_ts;
  }

  if (_cached_timestamp == timestamp)
  {
    // This has 2 usages:
    // 1. Any timestamps in seconds precision that are the same, we don't need to do anything.
    // 2. This checks happens after the _next_recalculation_timestamp calculation. The _next_recalculation_timestamp
    // will mutate _cached_timestamp and if they are similar we will just return here anyway
    return _pre_formatted_ts;
  }

  // Get the difference from our cached timestamp
  time_t const timestamp_diff = timestamp - _cached_timestamp;

  // cache this timestamp
  _cached_timestamp = timestamp;

  // Add the timestamp_diff to the cached seconds and calculate the new hours minutes seconds.
  // Note: cached_seconds could be in gmtime or localtime but we don't care as we are just
  // adding the difference.
  _cached_seconds += static_cast<uint32_t>(timestamp_diff);

  uint32_t cached_seconds = _cached_seconds;
  uint32_t const hours = cached_seconds / 3600;
  cached_seconds = cached_seconds % 3600;
  uint32_t const minutes = cached_seconds / 60;
  cached_seconds = cached_seconds % 60;
  uint32_t const seconds = cached_seconds;

  std::string to_replace;

  for (auto const& index : _cached_indexes)
  {
    // for each cached index we have, replace it when the new value
    switch (index.second)
    {
    case format_type::H:
      to_replace = std::to_string(hours);

      if (to_replace.size() == 1)
      {
        to_replace.insert(0, 1, '0');
      }

      _pre_formatted_ts.replace(index.first, 2, to_replace);
      break;
    case format_type::M:
      to_replace = std::to_string(minutes);

      if (to_replace.size() == 1)
      {
        to_replace.insert(0, 1, '0');
      }
      _pre_formatted_ts.replace(index.first, 2, to_replace);
      break;
    case format_type::S:
      to_replace = std::to_string(seconds);

      if (to_replace.size() == 1)
      {
        to_replace.insert(0, 1, '0');
      }
      _pre_formatted_ts.replace(index.first, 2, to_replace);
      break;
    case format_type::I:
      if (hours != 0)
      {
        to_replace = std::to_string(hours > 12 ? hours - 12 : hours);
      }
      else
      {
        // if hours is `00` we need to replace than with '12' instead in this format
        to_replace = std::to_string(12);
      }

      if (to_replace.size() == 1)
      {
        to_replace.insert(0, 1, '0');
      }
      _pre_formatted_ts.replace(index.first, 2, to_replace);
      break;
    case format_type::k:
      to_replace = std::to_string(hours);

      if (to_replace.size() == 1)
      {
        to_replace.insert(0, 1, ' ');
      }
      _pre_formatted_ts.replace(index.first, 2, to_replace);
      break;
    case format_type::l:
      if (hours != 0)
      {
        to_replace = std::to_string(hours > 12 ? hours - 12 : hours);
      }
      else
      {
        // if hours is `00` we need to replace than with '12' instead in this format
        to_replace = std::to_string(12);
      }

      if (to_replace.size() == 1)
      {
        to_replace.insert(0, 1, ' ');
      }
      _pre_formatted_ts.replace(index.first, 2, to_replace);
      break;
    case format_type::s:
      to_replace = std::to_string(_cached_timestamp);
      _pre_formatted_ts.replace(index.first, 10, to_replace);
      break;
    default:
      abort();
    }
  }

  return _pre_formatted_ts;
}

/***/
void StringFromTime::_populate_initial_parts(std::string timestamp_format)
{
  std::string part1;
  std::string part2;

  do
  {
    // we get part1 and part2 and keep looping on the new modified string without the part1 and
    // part2 until we find not %H, %M or %S at all
    auto const pp = _split_timestamp_format_once(timestamp_format);
    part1 = pp.first;
    part2 = pp.second;

    if (!part1.empty())
    {
      _initial_parts.push_back(part1);
    }

    if (!part2.empty())
    {
      _initial_parts.push_back(part2);
    }

    if (part1.empty() && part2.empty())
    {
      // if both part_1 and part_2 are empty it means we have no more
      // format modifiers to add, we push back the remaining timestamp_format string
      // and break
      if (!timestamp_format.empty())
      {
        _initial_parts.push_back(timestamp_format);
      }
      break;
    }
  } while (true);
}

void StringFromTime::_populate_pre_formatted_string_and_cached_indexes(time_t timestamp)
{
  _cached_timestamp = timestamp;

  tm time_info{};

  if (_time_zone == Timezone::LocalTime)
  {
    detail::localtime_rs(reinterpret_cast<time_t const*>(std::addressof(_cached_timestamp)),
                         std::addressof(time_info));
  }
  else if (_time_zone == Timezone::GmtTime)
  {
    detail::gmtime_rs(reinterpret_cast<time_t const*>(std::addressof(_cached_timestamp)),
                      std::addressof(time_info));
  }

  // also cache the seconds
  _cached_seconds =
    static_cast<uint32_t>((time_info.tm_hour * 3600) + (time_info.tm_min * 60) + time_info.tm_sec);

  // Now run through all parts and call strftime
  for (auto const& format_part : _initial_parts)
  {
    // We call strftime on each part of the timestamp to format it.
    _pre_formatted_ts += safe_strftime(format_part.data(), _cached_timestamp, _time_zone).data();

    // If we formatted and appended to the string a time modifier also store the
    // current index in the string
    if (format_part == "%H")
    {
      _cached_indexes.emplace_back(_pre_formatted_ts.size() - 2, format_type::H);
    }
    else if (format_part == "%M")
    {
      _cached_indexes.emplace_back(_pre_formatted_ts.size() - 2, format_type::M);
    }
    else if (format_part == "%S")
    {
      _cached_indexes.emplace_back(_pre_formatted_ts.size() - 2, format_type::S);
    }
    else if (format_part == "%I")
    {
      _cached_indexes.emplace_back(_pre_formatted_ts.size() - 2, format_type::I);
    }
    else if (format_part == "%k")
    {
      _cached_indexes.emplace_back(_pre_formatted_ts.size() - 2, format_type::k);
    }
    else if (format_part == "%l")
    {
      _cached_indexes.emplace_back(_pre_formatted_ts.size() - 2, format_type::l);
    }
    else if (format_part == "%s")
    {
      _cached_indexes.emplace_back(_pre_formatted_ts.size() - 10, format_type::s);
    }
  }
}

} // namespace detail
} // namespace quill