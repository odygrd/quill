#include "doctest/doctest.h"

#include "misc/DocTestExtensions.h"
#include "quill/backend/TimestampFormatter.h"
#include "quill/core/QuillError.h"

TEST_SUITE_BEGIN("TimestampFormatter");

using namespace quill::detail;

/***/
TEST_CASE("simple_format_string")
{
  // invalid format strings
#if !defined(QUILL_NO_EXCEPTIONS)
  REQUIRE_THROWS_AS(TimestampFormatter ts_formatter{"%I:%M%p%Qms%S%Qus z"}, quill::QuillError);
  REQUIRE_THROWS_AS(TimestampFormatter ts_formatter{"%I:%M%p%Qms%S%Qus%Qns z"}, quill::QuillError);
  REQUIRE_THROWS_AS(TimestampFormatter ts_formatter{"%I:%M%p%S%Qus%Qns z"}, quill::QuillError);
#endif

  // valid simple string
  REQUIRE_NOTHROW(TimestampFormatter ts_formatter{"%I:%M%p%S%Qns z"});
}

/***/
TEST_CASE("format_string_no_additional_specifier")
{
  std::chrono::nanoseconds constexpr timestamp{1587161887987654321};

  // simple formats without any ms/us/ns specifiers
  {
    TimestampFormatter ts_formatter{"%H:%M:%S", quill::Timezone::GmtTime};

    auto const& result = ts_formatter.format_timestamp(timestamp);
    REQUIRE_EQ(result, std::string_view{"22:18:07"});
  }

  {
    TimestampFormatter ts_formatter{"%F %H:%M:%S", quill::Timezone::GmtTime};

    auto const& result = ts_formatter.format_timestamp(timestamp);
    REQUIRE_EQ(result, std::string_view{"2020-04-17 22:18:07"});
  }

  // large simple string to cause reallocation
  {
    TimestampFormatter ts_formatter{"%A %B %d %T %Y %F", quill::Timezone::GmtTime};

    auto const& result = ts_formatter.format_timestamp(timestamp);
    REQUIRE_EQ(result, std::string_view{"Friday April 17 22:18:07 2020 2020-04-17"});
  }
}

/***/
TEST_CASE("format_string_with_millisecond_precision")
{
  // simple
  {
    std::chrono::nanoseconds constexpr timestamp{1587161887987654321};
    TimestampFormatter ts_formatter{"%H:%M:%S.%Qms", quill::Timezone::GmtTime};

    auto const& result = ts_formatter.format_timestamp(timestamp);
    REQUIRE_EQ(result, std::string_view{"22:18:07.987"});
  }

  // with double formatting
  {
    std::chrono::nanoseconds constexpr timestamp{1587161887803654321};
    TimestampFormatter ts_formatter{"%H:%M:%S.%Qms %D", quill::Timezone::GmtTime};

    auto const& result = ts_formatter.format_timestamp(timestamp);
    REQUIRE_EQ(result, std::string_view{"22:18:07.803 04/17/20"});
  }

  // with double formatting 2
  {
    std::chrono::nanoseconds constexpr timestamp{1587161887023654321};
    TimestampFormatter ts_formatter{"%H:%M:%S.%Qms-%G", quill::Timezone::GmtTime};

    auto const& result = ts_formatter.format_timestamp(timestamp);
    REQUIRE_EQ(result, std::string_view{"22:18:07.023-2020"});
  }

  // with zeros
  {
    std::chrono::nanoseconds constexpr timestamp{1587161887009654321};
    TimestampFormatter ts_formatter{"%H:%M:%S.%Qms", quill::Timezone::GmtTime};

    auto const& result = ts_formatter.format_timestamp(timestamp);
    REQUIRE_EQ(result, std::string_view{"22:18:07.009"});
  }
}

/***/
TEST_CASE("format_string_with_microsecond_precision")
{
  // simple
  {
    std::chrono::nanoseconds constexpr timestamp{1587161887987654321};
    TimestampFormatter ts_formatter{"%H:%M:%S.%Qus", quill::Timezone::GmtTime};

    auto const& result = ts_formatter.format_timestamp(timestamp);
    REQUIRE_EQ(result, std::string_view{"22:18:07.987654"});
  }

  // with double formatting
  {
    std::chrono::nanoseconds constexpr timestamp{1587161887803654321};
    TimestampFormatter ts_formatter{"%H:%M:%S.%Qus %D", quill::Timezone::GmtTime};

    auto const& result = ts_formatter.format_timestamp(timestamp);
    REQUIRE_EQ(result, std::string_view{"22:18:07.803654 04/17/20"});
  }

  // with double formatting 2
  {
    std::chrono::nanoseconds constexpr timestamp{1587161887010654321};
    TimestampFormatter ts_formatter{"%H:%M:%S.%Qus-%G", quill::Timezone::GmtTime};

    auto const& result = ts_formatter.format_timestamp(timestamp);
    REQUIRE_EQ(result, std::string_view{"22:18:07.010654-2020"});
  }

  // with zeros
  {
    std::chrono::nanoseconds constexpr timestamp{1587161887000004321};
    TimestampFormatter ts_formatter{"%H:%M:%S.%Qus", quill::Timezone::GmtTime};

    auto const& result = ts_formatter.format_timestamp(timestamp);
    REQUIRE_EQ(result, std::string_view{"22:18:07.000004"});
  }
}

/***/
TEST_CASE("format_string_with_nanosecond_precision")
{
  // simple
  {
    std::chrono::nanoseconds constexpr timestamp{1587161887987654321};
    TimestampFormatter ts_formatter{"%H:%M:%S.%Qns", quill::Timezone::GmtTime};

    auto const& result = ts_formatter.format_timestamp(timestamp);
    REQUIRE_EQ(result, std::string_view{"22:18:07.987654321"});
  }

  // with double formatting
  {
    std::chrono::nanoseconds constexpr timestamp{1587161887803654320};
    TimestampFormatter ts_formatter{"%H:%M:%S.%Qns %D", quill::Timezone::GmtTime};

    auto const& result = ts_formatter.format_timestamp(timestamp);
    REQUIRE_EQ(result, std::string_view{"22:18:07.803654320 04/17/20"});
  }

  // with double formatting 2
  {
    std::chrono::nanoseconds constexpr timestamp{1587161887000654321};
    TimestampFormatter ts_formatter{"%H:%M:%S.%Qns-%G", quill::Timezone::GmtTime};

    auto const& result = ts_formatter.format_timestamp(timestamp);
    REQUIRE_EQ(result, std::string_view{"22:18:07.000654321-2020"});
  }

  // with zeros
  {
    std::chrono::nanoseconds constexpr timestamp{1587161887000000009};
    TimestampFormatter ts_formatter{"%H:%M:%S.%Qns", quill::Timezone::GmtTime};

    auto const& result = ts_formatter.format_timestamp(timestamp);
    REQUIRE_EQ(result, std::string_view{"22:18:07.000000009"});
  }

  // with max
  {
    std::chrono::nanoseconds constexpr timestamp{1587161887999999999};
    TimestampFormatter ts_formatter{"%H:%M:%S.%Qns", quill::Timezone::GmtTime};

    auto const& result = ts_formatter.format_timestamp(timestamp);
    REQUIRE_EQ(result, std::string_view{"22:18:07.999999999"});
  }
}

TEST_SUITE_END();