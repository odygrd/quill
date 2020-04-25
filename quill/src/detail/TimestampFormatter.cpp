#include "quill/detail/TimestampFormatter.h"
#include "quill/QuillError.h"
#include "quill/detail/misc/Macros.h"
#include "quill/detail/misc/Os.h"
#include <cassert>
#include <ctime>

namespace
{
// Contains the additional specifier name, at the same index as the enum
std::array<char const*, 4> specifier_name{"", "%Qms", "%Qus", "%Qns"};

// All special specifiers have same length at the moment
constexpr size_t specifier_length = 4;

constexpr float _formatted_date_grow_factor = 1.5f;
} // namespace

namespace quill
{
namespace detail
{

/***/
TimestampFormatter::TimestampFormatter(std::string const& timestamp_format_string,
                                       Timezone timezone_type /* = Timezone::LocalTime */)
  : _timezone_type(timezone_type)
{
  // store the beginning of the found specifier
  size_t specifier_begin{std::string::npos};

  // look for all three special specifiers
  size_t search_qms = timestamp_format_string.find(specifier_name[AdditionalSpecifier::Qms]);
  if (search_qms != std::string::npos)
  {
    _additional_format_specifier = AdditionalSpecifier::Qms;
    specifier_begin = search_qms;
  }

  size_t search_qus = timestamp_format_string.find(specifier_name[AdditionalSpecifier::Qus]);
  if (search_qus != std::string::npos)
  {
    if (_additional_format_specifier != AdditionalSpecifier::None)
    {
      QUILL_THROW(QuillError{"format specifiers %Qms, %Qus and %Qns are mutually exclusive"});
    }

    _additional_format_specifier = AdditionalSpecifier::Qus;
    specifier_begin = search_qus;
  }

  size_t search_qns = timestamp_format_string.find(specifier_name[AdditionalSpecifier::Qns]);
  if (search_qns != std::string::npos)
  {
    if (_additional_format_specifier != AdditionalSpecifier::None)
    {
      QUILL_THROW(QuillError{"format specifiers %Qms, %Qus and %Qns are mutually exclusive"});
    }

    _additional_format_specifier = AdditionalSpecifier::Qns;
    specifier_begin = search_qns;
  }

  if (_additional_format_specifier == AdditionalSpecifier::None)
  {
    // If no additional specifier was found then we can simply store the whole format string
    _format_part_1 = timestamp_format_string;
  }
  else
  {
    // We now the index where the specifier begins so copy everything until there from beginning
    _format_part_1 = timestamp_format_string.substr(0, specifier_begin);

    // Now copy he remaining format string, ignoring the specifier
    size_t const specifier_end = specifier_begin + specifier_length;

    _format_part_2 =
      timestamp_format_string.substr(specifier_end, timestamp_format_string.length() - specifier_end);
  }
}

/***/
char const* TimestampFormatter::format_timestamp(std::chrono::nanoseconds time_since_epoch)
{
  int64_t const timestamp_ns = time_since_epoch.count();

  // convert timestamp to seconds
  int64_t const timestamp_secs = timestamp_ns / 1'000'000'000;

  // a timeinfo to hold the timestamp info
  tm timeinfo;

  assert((_timezone_type == Timezone::LocalTime || _timezone_type == Timezone::GmtTime) &&
         "Invalid timezone type");

  if (_timezone_type == Timezone::LocalTime)
  {
    detail::localtime_rs(reinterpret_cast<time_t const*>(std::addressof(timestamp_secs)),
                         std::addressof(timeinfo));
  }
  else if (_timezone_type == Timezone::GmtTime)
  {
    detail::gmtime_rs(reinterpret_cast<time_t const*>(std::addressof(timestamp_secs)), std::addressof(timeinfo));
  }

  // clear previous result to start formatting
  _formatted_date.clear();

  // 1. we always format part 1 - and store how many bytes we writted
  size_t formatted_ts_length = _strftime(timeinfo, 0, _format_part_1);

  // 2. We add any special ms/us/ns specifier if any
  auto get_extracted_ns = [timestamp_ns, timestamp_secs]() {
    return static_cast<uint32_t>(timestamp_ns - (timestamp_secs * 1'000'000'000));
  };

  if (_additional_format_specifier == AdditionalSpecifier::Qms)
  {
    auto const extracted_ms = get_extracted_ns() / 1'000'000;
    uint8_t constexpr fractional_seconds_size = 3;

    _append_fractional_seconds(formatted_ts_length, extracted_ms, fractional_seconds_size);

    // also append to length to know where to append part 2 if any
    formatted_ts_length += fractional_seconds_size;
  }
  else if (_additional_format_specifier == AdditionalSpecifier::Qus)
  {
    auto const extracted_us = get_extracted_ns() / 1'000;
    uint8_t constexpr fractional_seconds_size = 6;

    _append_fractional_seconds(formatted_ts_length, extracted_us, fractional_seconds_size);

    // also append to length to know where to append part 2 if any
    formatted_ts_length += fractional_seconds_size;
  }
  else if (_additional_format_specifier == AdditionalSpecifier::Qns)
  {
    auto const extracted_ns = get_extracted_ns();
    uint8_t constexpr fractional_seconds_size = 9;

    _append_fractional_seconds(formatted_ts_length, extracted_ns, fractional_seconds_size);

    // also append to length to know where to append part 2 if any
    formatted_ts_length += fractional_seconds_size;
  }

  // 3. format part 2 after fractional seconds - if any
  if (!_format_part_2.empty())
  {
    _strftime(timeinfo, formatted_ts_length, _format_part_2);
  }

  return _formatted_date.data();
}

/***/
size_t TimestampFormatter::_strftime(tm const& timeinfo, size_t formatted_date_pos, std::string const& format_string)
{
  auto res = strftime(&_formatted_date[formatted_date_pos], _formatted_date.capacity(),
                      format_string.data(), std::addressof(timeinfo));

  // if strftime fails we need to resize
  while (QUILL_UNLIKELY(res == 0))
  {
    _formatted_date.resize(
      static_cast<size_t>(static_cast<float>(_formatted_date.capacity()) * _formatted_date_grow_factor));
    res = strftime(&_formatted_date[formatted_date_pos], _formatted_date.capacity(),
                   format_string.data(), std::addressof(timeinfo));
  }

  return res;
}

/***/
void TimestampFormatter::_append_fractional_seconds(size_t formatted_timestamp_end,
                                                    uint32_t extracted_fractional_seconds,
                                                    uint8_t extracted_fractional_seconds_length)
{
  // check that we have enough size. We reserve an extra space for null terminator
  if (QUILL_UNLIKELY((formatted_timestamp_end + extracted_fractional_seconds_length + 1) >
                     _formatted_date.capacity()))
  {
    _formatted_date.resize(
      static_cast<size_t>(static_cast<float>(_formatted_date.capacity()) * _formatted_date_grow_factor));
  }

  // starting position to append the value
  size_t fractional_seconds_begin = formatted_timestamp_end;
  size_t const fractional_seconds_end = fractional_seconds_begin + extracted_fractional_seconds_length;

  // Fill with zeros the fractional seconds slots
  memset(&_formatted_date[fractional_seconds_begin], '0', extracted_fractional_seconds_length);
  fmt::format_int extracted_ms_string{extracted_fractional_seconds};

  // Get the real size we need to add based on extracted ms string
  fractional_seconds_begin = fractional_seconds_end - extracted_ms_string.size();
  memcpy(&_formatted_date[fractional_seconds_begin], extracted_ms_string.data(), extracted_ms_string.size());

  // Append a null terminator here, but we might overwrite it if there is a another call to strftime for the second part
  _formatted_date[fractional_seconds_end] = '\0';
}

} // namespace detail
} // namespace quill