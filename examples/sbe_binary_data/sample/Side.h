/* Generated SBE (Simple Binary Encoding) message codec */
#ifndef _SBE_SAMPLE_SIDE_CXX_H_
#define _SBE_SAMPLE_SIDE_CXX_H_

#if !defined(__STDC_LIMIT_MACROS)
  #define __STDC_LIMIT_MACROS 1
#endif

#include <cstdint>
#include <iomanip>
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>

#define SBE_NULLVALUE_INT8 (std::numeric_limits<std::int8_t>::min)()
#define SBE_NULLVALUE_INT16 (std::numeric_limits<std::int16_t>::min)()
#define SBE_NULLVALUE_INT32 (std::numeric_limits<std::int32_t>::min)()
#define SBE_NULLVALUE_INT64 (std::numeric_limits<std::int64_t>::min)()
#define SBE_NULLVALUE_UINT8 (std::numeric_limits<std::uint8_t>::max)()
#define SBE_NULLVALUE_UINT16 (std::numeric_limits<std::uint16_t>::max)()
#define SBE_NULLVALUE_UINT32 (std::numeric_limits<std::uint32_t>::max)()
#define SBE_NULLVALUE_UINT64 (std::numeric_limits<std::uint64_t>::max)()

namespace sbe
{
namespace sample
{

class Side
{
public:
  enum Value : uint8_t
  {
    BUY = static_cast<std::uint8_t>(1),
    SELL = static_cast<std::uint8_t>(2),
    NULL_VALUE = static_cast<std::uint8_t>(255)
  };

  static Side::Value get(std::uint8_t const value)
  {
    switch (value)
    {
    case static_cast<std::uint8_t>(1):
      return BUY;
    case static_cast<std::uint8_t>(2):
      return SELL;
    case static_cast<std::uint8_t>(255):
      return NULL_VALUE;
    }

    throw std::runtime_error("unknown value for enum Side [E103]");
  }

  static char const* c_str(Side::Value const value)
  {
    switch (value)
    {
    case BUY:
      return "BUY";
    case SELL:
      return "SELL";
    case NULL_VALUE:
      return "NULL_VALUE";
    }

    throw std::runtime_error("unknown value for enum Side [E103]:");
  }

  template <typename CharT, typename Traits>
  friend std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, Side::Value m)
  {
    return os << Side::c_str(m);
  }
};

} // namespace sample
} // namespace sbe

#endif
