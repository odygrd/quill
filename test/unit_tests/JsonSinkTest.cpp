#include "doctest/doctest.h"

#include "quill/sinks/JsonSink.h"
#include "quill/sinks/StreamSink.h"

#include "quill/bundled/fmt/format.h"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>
#include <string>
#include <string_view>
#include <vector>

TEST_SUITE_BEGIN("JsonSink");

namespace
{
/** Makes the helper functions of the JSON sinks callable */
class JsonSinkHelpers : public quill::detail::JsonSink<quill::StreamSink>
{
public:
  static std::string escape(std::string_view value)
  {
    fmtquill::memory_buffer buffer;
    append_json_escaped(buffer, value);
    return std::string{buffer.data(), buffer.size()};
  }

  static std::string decimal(uint64_t value)
  {
    fmtquill::memory_buffer buffer;
    _append_decimal(buffer, value);
    return std::string{buffer.data(), buffer.size()};
  }
};

std::string to_chars(uint64_t value)
{
  char buffer[std::numeric_limits<uint64_t>::digits10 + 1u];
  auto const result = std::to_chars(buffer, buffer + sizeof(buffer), value);
  return std::string{buffer, result.ptr};
}

/** Escapes one byte at a time */
std::string reference_escape(std::string_view value)
{
  std::string escaped;

  for (size_t i = 0; i < value.size(); ++i)
  {
    unsigned char const c = static_cast<unsigned char>(value[i]);
    bool const is_separator = (c == 0xE2) && ((i + 2) < value.size()) && (value[i + 1] == '\x80') &&
      ((value[i + 2] == '\xA8') || (value[i + 2] == '\xA9'));

    if (c == '"')
    {
      escaped += "\\\"";
    }
    else if (c == '\\')
    {
      escaped += "\\\\";
    }
    else if (c == '\b')
    {
      escaped += "\\b";
    }
    else if (c == '\f')
    {
      escaped += "\\f";
    }
    else if (c == '\n')
    {
      escaped += "\\n";
    }
    else if (c == '\r')
    {
      escaped += "\\r";
    }
    else if (c == '\t')
    {
      escaped += "\\t";
    }
    else if (c < 0x20)
    {
      escaped += fmtquill::format("\\u{:04X}", c);
    }
    else if (is_separator)
    {
      escaped += (value[i + 2] == '\xA8') ? "\\u2028" : "\\u2029";
      i += 2;
    }
    else
    {
      escaped += static_cast<char>(c);
    }
  }

  return escaped;
}
} // namespace

/***/
TEST_CASE("json_escape_escapes_each_byte_value")
{
  for (size_t c = 0; c < 256; ++c)
  {
    std::string value{"text "};
    value += static_cast<char>(c);
    value += " more text";

    REQUIRE_EQ(JsonSinkHelpers::escape(value), reference_escape(value));
  }
}

/***/
TEST_CASE("json_escape_finds_escapes_at_each_position")
{
  // At each position of strings shorter and longer than the 8 bytes that are checked together
  std::vector<std::string> const specials{"\"",       "\\",           "\n",           "\x01",
                                          "\x1f",     "\xE2\x80\xA8", "\xE2\x80\xA9", "\xE2",
                                          "\xE2\x80", "\xE2\x80\x41", "\xC3\xBC"};

  for (size_t size = 0; size <= 40; ++size)
  {
    for (size_t pos = 0; pos <= size; ++pos)
    {
      for (std::string const& special : specials)
      {
        std::string value(size, 'x');
        value.insert(pos, special);

        REQUIRE_EQ(JsonSinkHelpers::escape(value), reference_escape(value));
      }
    }
  }
}

/***/
TEST_CASE("json_escape_matches_the_reference_on_random_strings")
{
  std::mt19937 rng{42};
  std::string const alphabet{"ab {}:\"\\\n\t\x01\x1f\x7f\x80\xA8\xA9\xE2\xC3\xBC\xFF"};

  for (size_t round = 0; round < 20000; ++round)
  {
    std::string value;
    size_t const size = rng() % 64;

    for (size_t i = 0; i < size; ++i)
    {
      // Mostly plain text, as in log messages
      value += ((rng() % 4) == 0) ? alphabet[rng() % alphabet.size()]
                                  : static_cast<char>('a' + (rng() % 26));
    }

    REQUIRE_EQ(JsonSinkHelpers::escape(value), reference_escape(value));
  }
}

/***/
TEST_CASE("json_sink_formats_decimals_as_to_chars")
{
  std::vector<uint64_t> values{0, (std::numeric_limits<uint64_t>::max)()};

  // Each power of ten and its neighbours, where the number of digits changes
  for (uint64_t power = 1; power <= 1'000'000'000'000'000'000ull; power *= 10)
  {
    values.push_back(power - 1);
    values.push_back(power);
    values.push_back(power + 1);
  }

  std::mt19937_64 rng{7};
  for (size_t i = 0; i < 20000; ++i)
  {
    // Random values of each length
    values.push_back(rng() >> (rng() % 64));
  }

  for (uint64_t const value : values)
  {
    REQUIRE_EQ(JsonSinkHelpers::decimal(value), to_chars(value));
  }
}

TEST_SUITE_END();
