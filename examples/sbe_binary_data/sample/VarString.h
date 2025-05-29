/* Generated SBE (Simple Binary Encoding) message codec */
#ifndef _SBE_SAMPLE_VARSTRING_CXX_H_
#define _SBE_SAMPLE_VARSTRING_CXX_H_

#if __cplusplus >= 201103L
  #define SBE_CONSTEXPR constexpr
  #define SBE_NOEXCEPT noexcept
#else
  #define SBE_CONSTEXPR
  #define SBE_NOEXCEPT
#endif

#if __cplusplus >= 201703L
  #include <string_view>
  #define SBE_NODISCARD [[nodiscard]]
  #if !defined(SBE_USE_STRING_VIEW)
    #define SBE_USE_STRING_VIEW 1
  #endif
#else
  #define SBE_NODISCARD
#endif

#if __cplusplus >= 202002L
  #include <span>
  #if !defined(SBE_USE_SPAN)
    #define SBE_USE_SPAN 1
  #endif
#endif

#if !defined(__STDC_LIMIT_MACROS)
  #define __STDC_LIMIT_MACROS 1
#endif

#include <cstdint>
#include <cstring>
#include <iomanip>
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#if defined(WIN32) || defined(_WIN32)
  #define SBE_BIG_ENDIAN_ENCODE_16(v) _byteswap_ushort(v)
  #define SBE_BIG_ENDIAN_ENCODE_32(v) _byteswap_ulong(v)
  #define SBE_BIG_ENDIAN_ENCODE_64(v) _byteswap_uint64(v)
  #define SBE_LITTLE_ENDIAN_ENCODE_16(v) (v)
  #define SBE_LITTLE_ENDIAN_ENCODE_32(v) (v)
  #define SBE_LITTLE_ENDIAN_ENCODE_64(v) (v)
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  #define SBE_BIG_ENDIAN_ENCODE_16(v) __builtin_bswap16(v)
  #define SBE_BIG_ENDIAN_ENCODE_32(v) __builtin_bswap32(v)
  #define SBE_BIG_ENDIAN_ENCODE_64(v) __builtin_bswap64(v)
  #define SBE_LITTLE_ENDIAN_ENCODE_16(v) (v)
  #define SBE_LITTLE_ENDIAN_ENCODE_32(v) (v)
  #define SBE_LITTLE_ENDIAN_ENCODE_64(v) (v)
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  #define SBE_LITTLE_ENDIAN_ENCODE_16(v) __builtin_bswap16(v)
  #define SBE_LITTLE_ENDIAN_ENCODE_32(v) __builtin_bswap32(v)
  #define SBE_LITTLE_ENDIAN_ENCODE_64(v) __builtin_bswap64(v)
  #define SBE_BIG_ENDIAN_ENCODE_16(v) (v)
  #define SBE_BIG_ENDIAN_ENCODE_32(v) (v)
  #define SBE_BIG_ENDIAN_ENCODE_64(v) (v)
#else
  #error                                                                                           \
    "Byte Ordering of platform not determined. Set __BYTE_ORDER__ manually before including this file."
#endif

#if !defined(SBE_BOUNDS_CHECK_EXPECT)
  #if defined(SBE_NO_BOUNDS_CHECK)
    #define SBE_BOUNDS_CHECK_EXPECT(exp, c) (false)
  #elif defined(_MSC_VER)
    #define SBE_BOUNDS_CHECK_EXPECT(exp, c) (exp)
  #else
    #define SBE_BOUNDS_CHECK_EXPECT(exp, c) (__builtin_expect(exp, c))
  #endif

#endif

#define SBE_FLOAT_NAN std::numeric_limits<float>::quiet_NaN()
#define SBE_DOUBLE_NAN std::numeric_limits<double>::quiet_NaN()
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

class VarString
{
private:
  char* m_buffer = nullptr;
  std::uint64_t m_bufferLength = 0;
  std::uint64_t m_offset = 0;
  std::uint64_t m_actingVersion = 0;

public:
  enum MetaAttribute
  {
    EPOCH,
    TIME_UNIT,
    SEMANTIC_TYPE,
    PRESENCE
  };

  union sbe_float_as_uint_u
  {
    float fp_value;
    std::uint32_t uint_value;
  };

  union sbe_double_as_uint_u
  {
    double fp_value;
    std::uint64_t uint_value;
  };

  VarString() = default;

  VarString(char* buffer, std::uint64_t const offset, std::uint64_t const bufferLength, std::uint64_t const actingVersion)
    : m_buffer(buffer), m_bufferLength(bufferLength), m_offset(offset), m_actingVersion(actingVersion)
  {
    if (SBE_BOUNDS_CHECK_EXPECT(((m_offset + -1) > m_bufferLength), false))
    {
      throw std::runtime_error("buffer too short for flyweight [E107]");
    }
  }

  VarString(char* buffer, std::uint64_t const bufferLength, std::uint64_t const actingVersion)
    : VarString(buffer, 0, bufferLength, actingVersion)
  {
  }

  VarString(char* buffer, std::uint64_t const bufferLength)
    : VarString(buffer, 0, bufferLength, sbeSchemaVersion())
  {
  }

  VarString& wrap(char* buffer, std::uint64_t const offset, std::uint64_t const actingVersion,
                  std::uint64_t const bufferLength)
  {
    m_buffer = buffer;
    m_bufferLength = bufferLength;
    m_offset = offset;
    m_actingVersion = actingVersion;

    if (SBE_BOUNDS_CHECK_EXPECT(((m_offset + -1) > m_bufferLength), false))
    {
      throw std::runtime_error("buffer too short for flyweight [E107]");
    }

    return *this;
  }

  SBE_NODISCARD static SBE_CONSTEXPR std::uint64_t encodedLength() SBE_NOEXCEPT { return -1; }

  SBE_NODISCARD std::uint64_t offset() const SBE_NOEXCEPT { return m_offset; }

  SBE_NODISCARD const char* buffer() const SBE_NOEXCEPT { return m_buffer; }

  SBE_NODISCARD char* buffer() SBE_NOEXCEPT { return m_buffer; }

  SBE_NODISCARD std::uint64_t bufferLength() const SBE_NOEXCEPT { return m_bufferLength; }

  SBE_NODISCARD std::uint64_t actingVersion() const SBE_NOEXCEPT { return m_actingVersion; }

  SBE_NODISCARD static SBE_CONSTEXPR std::uint16_t sbeSchemaId() SBE_NOEXCEPT
  {
    return static_cast<std::uint16_t>(1);
  }

  SBE_NODISCARD static SBE_CONSTEXPR std::uint16_t sbeSchemaVersion() SBE_NOEXCEPT
  {
    return static_cast<std::uint16_t>(1);
  }

  SBE_NODISCARD static const char* lengthMetaAttribute(MetaAttribute const metaAttribute) SBE_NOEXCEPT
  {
    switch (metaAttribute)
    {
    case MetaAttribute::PRESENCE:
      return "required";
    default:
      return "";
    }
  }

  static SBE_CONSTEXPR std::uint16_t lengthId() SBE_NOEXCEPT { return -1; }

  SBE_NODISCARD static SBE_CONSTEXPR std::uint64_t lengthSinceVersion() SBE_NOEXCEPT { return 0; }

  SBE_NODISCARD bool lengthInActingVersion() SBE_NOEXCEPT { return true; }

  SBE_NODISCARD static SBE_CONSTEXPR std::size_t lengthEncodingOffset() SBE_NOEXCEPT { return 0; }

  static SBE_CONSTEXPR std::uint8_t lengthNullValue() SBE_NOEXCEPT { return SBE_NULLVALUE_UINT8; }

  static SBE_CONSTEXPR std::uint8_t lengthMinValue() SBE_NOEXCEPT
  {
    return static_cast<std::uint8_t>(0);
  }

  static SBE_CONSTEXPR std::uint8_t lengthMaxValue() SBE_NOEXCEPT
  {
    return static_cast<std::uint8_t>(40);
  }

  static SBE_CONSTEXPR std::size_t lengthEncodingLength() SBE_NOEXCEPT { return 1; }

  SBE_NODISCARD std::uint8_t length() const SBE_NOEXCEPT
  {
    std::uint8_t val;
    std::memcpy(&val, m_buffer + m_offset + 0, sizeof(std::uint8_t));
    return (val);
  }

  VarString& length(std::uint8_t const value) SBE_NOEXCEPT
  {
    std::uint8_t val = (value);
    std::memcpy(m_buffer + m_offset + 0, &val, sizeof(std::uint8_t));
    return *this;
  }

  SBE_NODISCARD static const char* varDataMetaAttribute(MetaAttribute const metaAttribute) SBE_NOEXCEPT
  {
    switch (metaAttribute)
    {
    case MetaAttribute::PRESENCE:
      return "required";
    default:
      return "";
    }
  }

  static SBE_CONSTEXPR std::uint16_t varDataId() SBE_NOEXCEPT { return -1; }

  SBE_NODISCARD static SBE_CONSTEXPR std::uint64_t varDataSinceVersion() SBE_NOEXCEPT { return 0; }

  SBE_NODISCARD bool varDataInActingVersion() SBE_NOEXCEPT { return true; }

  SBE_NODISCARD static SBE_CONSTEXPR std::size_t varDataEncodingOffset() SBE_NOEXCEPT { return 1; }

  static SBE_CONSTEXPR char varDataNullValue() SBE_NOEXCEPT { return static_cast<char>(0); }

  static SBE_CONSTEXPR char varDataMinValue() SBE_NOEXCEPT { return static_cast<char>(32); }

  static SBE_CONSTEXPR char varDataMaxValue() SBE_NOEXCEPT { return static_cast<char>(126); }

  static SBE_CONSTEXPR std::size_t varDataEncodingLength() SBE_NOEXCEPT { return -1; }

  template <typename CharT, typename Traits>
  friend std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& builder, VarString& writer)
  {
    builder << '{';
    builder << R"("length": )";
    builder << +writer.length();

    builder << '}';

    return builder;
  }
};

} // namespace sample
} // namespace sbe

#endif
