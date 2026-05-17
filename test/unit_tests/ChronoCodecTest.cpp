#include "doctest/doctest.h"

#include "quill/std/Chrono.h"

#include "quill/bundled/fmt/format.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <type_traits>
#include <vector>

TEST_SUITE_BEGIN("ChronoCodec");

using namespace quill;

namespace
{
template <typename T>
T roundtrip_through_codec(T const& value)
{
  detail::SizeCacheVector size_cache;
  size_t const encoded_size = Codec<T>::compute_encoded_size(size_cache, value);

  std::vector<std::byte> buffer(encoded_size);
  std::byte* write_ptr = buffer.data();
  uint32_t conditional_arg_size_cache_index{0};

  Codec<T>::encode(write_ptr, size_cache, conditional_arg_size_cache_index, value);
  REQUIRE_EQ(static_cast<size_t>(write_ptr - buffer.data()), encoded_size);

  std::byte* read_ptr = buffer.data();
  T decoded_value = Codec<T>::decode_arg(read_ptr);
  REQUIRE_EQ(static_cast<size_t>(read_ptr - buffer.data()), encoded_size);
  return decoded_value;
}

template <typename T>
void require_roundtrip(T const& value)
{
  static_assert(std::is_trivially_copyable_v<T>, "Expected trivially copyable chrono type");
  T const decoded_value = roundtrip_through_codec(value);
  REQUIRE(decoded_value == value);
}
} // namespace

TEST_CASE("chrono_duration_and_time_point_roundtrip")
{
  require_roundtrip(std::chrono::milliseconds{1234});
  require_roundtrip(std::chrono::system_clock::time_point{std::chrono::milliseconds{5678}});
}

TEST_CASE("std_tm_roundtrip")
{
  std::tm value{};
  value.tm_sec = 45;
  value.tm_min = 30;
  value.tm_hour = 12;
  value.tm_mday = 9;
  value.tm_mon = 7;    // August (0-based)
  value.tm_year = 124; // 2024
  value.tm_wday = 5;   // Friday
  value.tm_yday = 221;
  value.tm_isdst = 0;

  static_assert(std::is_trivially_copyable_v<std::tm>,
                "std::tm is expected to be trivially copyable");

  std::tm const decoded = roundtrip_through_codec(value);
  REQUIRE_EQ(decoded.tm_sec, value.tm_sec);
  REQUIRE_EQ(decoded.tm_min, value.tm_min);
  REQUIRE_EQ(decoded.tm_hour, value.tm_hour);
  REQUIRE_EQ(decoded.tm_mday, value.tm_mday);
  REQUIRE_EQ(decoded.tm_mon, value.tm_mon);
  REQUIRE_EQ(decoded.tm_year, value.tm_year);
  REQUIRE_EQ(decoded.tm_wday, value.tm_wday);
  REQUIRE_EQ(decoded.tm_yday, value.tm_yday);
  REQUIRE_EQ(decoded.tm_isdst, value.tm_isdst);

  // Sanity check that fmt can format std::tm through its native formatter.
  REQUIRE_FALSE(fmtquill::format("{:%F %T}", value).empty());
}

TEST_CASE("chrono_cxx20_calendar_types_roundtrip")
{
#if QUILL_HAS_CXX20_CHRONO
  std::chrono::year const year_value{2024};
  std::chrono::month const month_value{8};
  std::chrono::day const day_value{9};
  std::chrono::weekday const weekday_value{5};
  std::chrono::year_month_day const year_month_day_value{year_value, month_value, day_value};

  require_roundtrip(year_value);
  require_roundtrip(month_value);
  require_roundtrip(day_value);
  require_roundtrip(weekday_value);
  require_roundtrip(year_month_day_value);

  // Sanity check that fmt can format each of the round-tripped values.
  REQUIRE_FALSE(fmtquill::format("{}", year_value).empty());
  REQUIRE_FALSE(fmtquill::format("{}", month_value).empty());
  REQUIRE_FALSE(fmtquill::format("{}", day_value).empty());
  REQUIRE_FALSE(fmtquill::format("{}", weekday_value).empty());
  REQUIRE_FALSE(fmtquill::format("{}", year_month_day_value).empty());
#else
  MESSAGE("QUILL_HAS_CXX20_CHRONO is 0 — skipping C++20 calendar codec tests");
#endif
}

TEST_SUITE_END();
