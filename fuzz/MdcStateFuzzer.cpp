// MdcStateFuzzer - Fuzzes MDC format parsing and state transitions
//
// This target exercises:
// - arbitrary BackendOptions::mdc_format_pattern strings
// - repeated set/erase/clear sequences
// - delayed rebuild_formatted_mdc() calls after multiple mutations
//
// The fuzzer keeps a simple reference map and checks that rebuilt MDC output
// matches the expected rendered string.

#include "FuzzDataExtractor.h"
#include "quill/backend/BackendMdcState.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <string>
#include <string_view>

namespace
{
struct FormatParts
{
  std::string prefix;
  std::string key_value_separator;
  std::string field_separator;
  std::string suffix;
};

bool try_parse_format_pattern(std::string_view pattern, FormatParts& format_parts)
{
  static constexpr std::string_view placeholder{"{}"};

  size_t const first = pattern.find(placeholder);
  if (first == std::string_view::npos)
  {
    return false;
  }

  size_t const second = pattern.find(placeholder, first + placeholder.size());
  if (second == std::string_view::npos)
  {
    return false;
  }

  if (pattern.find(placeholder, second + placeholder.size()) != std::string_view::npos)
  {
    return false;
  }

  size_t const after_second = second + placeholder.size();
  if (after_second >= pattern.size())
  {
    return false;
  }

  format_parts.prefix.assign(pattern.data(), first);
  format_parts.key_value_separator.assign(pattern.data() + first + placeholder.size(),
                                          second - first - placeholder.size());
  format_parts.field_separator.assign(pattern.data() + after_second, pattern.size() - after_second - 1u);
  format_parts.suffix.assign(pattern.data() + pattern.size() - 1u, 1u);
  return true;
}

std::string build_expected_mdc(std::map<std::string, std::string> const& fields, FormatParts const& format_parts)
{
  if (fields.empty())
  {
    return {};
  }

  std::string result;
  result.append(format_parts.prefix);

  size_t i = 0;
  for (auto const& [key, value] : fields)
  {
    result.append(key);
    result.append(format_parts.key_value_separator);
    result.append(value);

    if (++i != fields.size())
    {
      result.append(format_parts.field_separator);
    }
  }

  result.append(format_parts.suffix);
  return result;
}

void require(bool condition)
{
  if (!condition)
  {
    std::abort();
  }
}

void validate_state(quill::detail::BackendMdcState& state,
                    std::map<std::string, std::string> const& reference, FormatParts const& format_parts)
{
  state.rebuild_formatted_mdc();

  std::string const expected = build_expected_mdc(reference, format_parts);
  require(state.empty() == reference.empty());
  require(state.formatted_mdc() == expected);

  state.rebuild_formatted_mdc();
  require(state.formatted_mdc() == expected);
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size)
{
  if (size == 0)
  {
    return 0;
  }

  FuzzDataExtractor extractor{data, size};

  std::string const pattern = extractor.get_string(64);
  FormatParts format_parts;
  bool const valid_pattern = try_parse_format_pattern(pattern, format_parts);

  quill::detail::BackendMdcState state{" [{}: {}, ]"};
  try
  {
    state = quill::detail::BackendMdcState{pattern};
    require(valid_pattern);
  }
  catch (quill::QuillError const&)
  {
    require(!valid_pattern);
    return 0;
  }

  std::map<std::string, std::string> reference;

  while (extractor.has_data())
  {
    switch (extractor.get_byte() % 6u)
    {
    case 0:
    {
      std::string const key = extractor.get_string(24);
      std::string const value = extractor.get_string(32);
      state.set(key, value);
      reference[key] = value;
      break;
    }
    case 1:
    {
      std::string const key = extractor.get_string(24);
      state.erase(key);
      reference.erase(key);
      break;
    }
    case 2:
    {
      state.clear();
      reference.clear();
      require(state.empty());
      require(state.formatted_mdc().empty());
      break;
    }
    case 3:
    {
      validate_state(state, reference, format_parts);
      break;
    }
    case 4:
    {
      size_t const count = (extractor.get_byte() % 4u) + 1u;
      for (size_t i = 0; i < count && extractor.has_data(); ++i)
      {
        std::string const key = extractor.get_string(24);
        std::string const value = extractor.get_string(32);
        state.set(key, value);
        reference[key] = value;
      }
      break;
    }
    case 5:
    {
      size_t const count = (extractor.get_byte() % 4u) + 1u;
      for (size_t i = 0; i < count && extractor.has_data(); ++i)
      {
        std::string const key = extractor.get_string(24);
        state.erase(key);
        reference.erase(key);
      }
      break;
    }
    default:
      break;
    }
  }

  validate_state(state, reference, format_parts);
  return 0;
}
