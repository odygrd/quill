#include "quill/detail/backend/TimestampFormatter.h"
#include "quill/Fmt.h"                // for buffer
#include "quill/QuillError.h"         // for QUILL_THROW, QuillError
#include "quill/detail/misc/Common.h" // for QUILL_UNLIKELY
#include "quill/detail/misc/Os.h"     // for gmtime_rs, localtime_rs
#include <array>                      // for array
#include <cassert>                    // for assert
#include <cstring>                    // for memcpy, memset
#include <ctime>                      // for size_t, strftime, time_t
#include <utility>                    // for addressof

namespace
{
// Contains the additional specifier name, at the same index as the enum
std::array<char const*, 4> specifier_name{"", "%Qms", "%Qus", "%Qns"};

// All special specifiers have same length at the moment
constexpr size_t specifier_length = 4;
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
  assert((_timezone_type == Timezone::LocalTime || _timezone_type == Timezone::GmtTime) &&
         "Invalid timezone type");

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

  // Now init two custom string from time classes with pre-formatted strings
  _strftime_part_1.init(_format_part_1, _timezone_type);
  _strftime_part_2.init(_format_part_2, _timezone_type);
}

/***/
std::string_view TimestampFormatter::format_timestamp(std::chrono::nanoseconds time_since_epoch)
{
  int64_t const timestamp_ns = time_since_epoch.count();

  // convert timestamp to seconds
  int64_t const timestamp_secs = timestamp_ns / 1'000'000'000;

  // First always clear our cached string
  _formatted_date.clear();

  // 1. we always format part 1
  _formatted_date += _strftime_part_1.format_timestamp(timestamp_secs);

  // 2. We add any special ms/us/ns specifier if any
  auto const extracted_ns = static_cast<uint32_t>(timestamp_ns - (timestamp_secs * 1'000'000'000));

  if (_additional_format_specifier == AdditionalSpecifier::Qms)
  {
    uint32_t const extracted_ms = extracted_ns / 1'000'000;
    constexpr char const* zeros = "000";

    // Add as many zeros as the extracted_fractional_seconds_length
    _formatted_date += zeros;

    _append_fractional_seconds(extracted_ms);
  }
  else if (_additional_format_specifier == AdditionalSpecifier::Qus)
  {
    uint32_t const extracted_us = extracted_ns / 1'000;
    constexpr char const* zeros = "000000";

    // Add as many zeros as the extracted_fractional_seconds_length
    _formatted_date += zeros;

    _append_fractional_seconds(extracted_us);
  }
  else if (_additional_format_specifier == AdditionalSpecifier::Qns)
  {
    constexpr char const* zeros = "000000000";

    // Add as many zeros as the extracted_fractional_seconds_length
    _formatted_date += zeros;

    _append_fractional_seconds(extracted_ns);
  }

  // 3. format part 2 after fractional seconds - if any
  if (!_format_part_2.empty())
  {
    _formatted_date += _strftime_part_2.format_timestamp(timestamp_secs);
  }

  return _formatted_date.data();
}

/***/
void TimestampFormatter::_append_fractional_seconds(uint32_t extracted_fractional_seconds)
{
  // Format the seconds and add them
  fmt::format_int extracted_ms_string{extracted_fractional_seconds};

  // _formatted_date.size() - extracted_ms_string.size() is where we want to begin placing the fractional seconds
  memcpy(&_formatted_date[_formatted_date.size() - extracted_ms_string.size()],
         extracted_ms_string.data(), extracted_ms_string.size());
}

} // namespace detail
} // namespace quill