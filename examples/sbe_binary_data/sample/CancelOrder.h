/* Generated SBE (Simple Binary Encoding) message codec */
#ifndef _SBE_SAMPLE_CANCELORDER_CXX_H_
#define _SBE_SAMPLE_CANCELORDER_CXX_H_

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

#include "MessageHeader.h"
#include "Side.h"
#include "VarString.h"

namespace sbe
{
namespace sample
{

class CancelOrder
{
private:
  char* m_buffer = nullptr;
  std::uint64_t m_bufferLength = 0;
  std::uint64_t m_offset = 0;
  std::uint64_t m_position = 0;
  std::uint64_t m_actingBlockLength = 0;
  std::uint64_t m_actingVersion = 0;

  inline std::uint64_t* sbePositionPtr() SBE_NOEXCEPT { return &m_position; }

public:
  static constexpr std::uint16_t SBE_BLOCK_LENGTH = static_cast<std::uint16_t>(20);
  static constexpr std::uint16_t SBE_TEMPLATE_ID = static_cast<std::uint16_t>(2);
  static constexpr std::uint16_t SBE_SCHEMA_ID = static_cast<std::uint16_t>(1);
  static constexpr std::uint16_t SBE_SCHEMA_VERSION = static_cast<std::uint16_t>(1);
  static constexpr char const* SBE_SEMANTIC_VERSION = "1.0.0";

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

  using messageHeader = MessageHeader;

  CancelOrder() = default;

  CancelOrder(char* buffer, std::uint64_t const offset, std::uint64_t const bufferLength,
              std::uint64_t const actingBlockLength, std::uint64_t const actingVersion)
    : m_buffer(buffer),
      m_bufferLength(bufferLength),
      m_offset(offset),
      m_position(sbeCheckPosition(offset + actingBlockLength)),
      m_actingBlockLength(actingBlockLength),
      m_actingVersion(actingVersion)
  {
  }

  CancelOrder(char* buffer, std::uint64_t const bufferLength)
    : CancelOrder(buffer, 0, bufferLength, sbeBlockLength(), sbeSchemaVersion())
  {
  }

  CancelOrder(char* buffer, std::uint64_t const bufferLength, std::uint64_t const actingBlockLength,
              std::uint64_t const actingVersion)
    : CancelOrder(buffer, 0, bufferLength, actingBlockLength, actingVersion)
  {
  }

  SBE_NODISCARD static SBE_CONSTEXPR std::uint16_t sbeBlockLength() SBE_NOEXCEPT
  {
    return static_cast<std::uint16_t>(20);
  }

  SBE_NODISCARD static SBE_CONSTEXPR std::uint64_t sbeBlockAndHeaderLength() SBE_NOEXCEPT
  {
    return messageHeader::encodedLength() + sbeBlockLength();
  }

  SBE_NODISCARD static SBE_CONSTEXPR std::uint16_t sbeTemplateId() SBE_NOEXCEPT
  {
    return static_cast<std::uint16_t>(2);
  }

  SBE_NODISCARD static SBE_CONSTEXPR std::uint16_t sbeSchemaId() SBE_NOEXCEPT
  {
    return static_cast<std::uint16_t>(1);
  }

  SBE_NODISCARD static SBE_CONSTEXPR std::uint16_t sbeSchemaVersion() SBE_NOEXCEPT
  {
    return static_cast<std::uint16_t>(1);
  }

  SBE_NODISCARD static const char* sbeSemanticVersion() SBE_NOEXCEPT { return "1.0.0"; }

  SBE_NODISCARD static SBE_CONSTEXPR const char* sbeSemanticType() SBE_NOEXCEPT { return ""; }

  SBE_NODISCARD std::uint64_t offset() const SBE_NOEXCEPT { return m_offset; }

  CancelOrder& wrapForEncode(char* buffer, std::uint64_t const offset, std::uint64_t const bufferLength)
  {
    m_buffer = buffer;
    m_bufferLength = bufferLength;
    m_offset = offset;
    m_actingBlockLength = sbeBlockLength();
    m_actingVersion = sbeSchemaVersion();
    m_position = sbeCheckPosition(m_offset + m_actingBlockLength);
    return *this;
  }

  CancelOrder& wrapAndApplyHeader(char* buffer, std::uint64_t const offset, std::uint64_t const bufferLength)
  {
    messageHeader hdr(buffer, offset, bufferLength, sbeSchemaVersion());

    hdr.blockLength(sbeBlockLength()).templateId(sbeTemplateId()).schemaId(sbeSchemaId()).version(sbeSchemaVersion());

    m_buffer = buffer;
    m_bufferLength = bufferLength;
    m_offset = offset + messageHeader::encodedLength();
    m_actingBlockLength = sbeBlockLength();
    m_actingVersion = sbeSchemaVersion();
    m_position = sbeCheckPosition(m_offset + m_actingBlockLength);
    return *this;
  }

  CancelOrder& wrapForDecode(char* buffer, std::uint64_t const offset, std::uint64_t const actingBlockLength,
                             std::uint64_t const actingVersion, std::uint64_t const bufferLength)
  {
    m_buffer = buffer;
    m_bufferLength = bufferLength;
    m_offset = offset;
    m_actingBlockLength = actingBlockLength;
    m_actingVersion = actingVersion;
    m_position = sbeCheckPosition(m_offset + m_actingBlockLength);
    return *this;
  }

  CancelOrder& sbeRewind()
  {
    return wrapForDecode(m_buffer, m_offset, m_actingBlockLength, m_actingVersion, m_bufferLength);
  }

  SBE_NODISCARD std::uint64_t sbePosition() const SBE_NOEXCEPT { return m_position; }

  // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
  std::uint64_t sbeCheckPosition(std::uint64_t const position)
  {
    if (SBE_BOUNDS_CHECK_EXPECT((position > m_bufferLength), false))
    {
      throw std::runtime_error("buffer too short [E100]");
    }
    return position;
  }

  void sbePosition(std::uint64_t const position) { m_position = sbeCheckPosition(position); }

  SBE_NODISCARD std::uint64_t encodedLength() const SBE_NOEXCEPT
  {
    return sbePosition() - m_offset;
  }

  SBE_NODISCARD std::uint64_t decodeLength() const
  {
    CancelOrder skipper(m_buffer, m_offset, m_bufferLength, m_actingBlockLength, m_actingVersion);
    skipper.skip();
    return skipper.encodedLength();
  }

  SBE_NODISCARD const char* buffer() const SBE_NOEXCEPT { return m_buffer; }

  SBE_NODISCARD char* buffer() SBE_NOEXCEPT { return m_buffer; }

  SBE_NODISCARD std::uint64_t bufferLength() const SBE_NOEXCEPT { return m_bufferLength; }

  SBE_NODISCARD std::uint64_t actingVersion() const SBE_NOEXCEPT { return m_actingVersion; }

  SBE_NODISCARD static const char* orderIdMetaAttribute(MetaAttribute const metaAttribute) SBE_NOEXCEPT
  {
    switch (metaAttribute)
    {
    case MetaAttribute::PRESENCE:
      return "required";
    default:
      return "";
    }
  }

  static SBE_CONSTEXPR std::uint16_t orderIdId() SBE_NOEXCEPT { return 1; }

  SBE_NODISCARD static SBE_CONSTEXPR std::uint64_t orderIdSinceVersion() SBE_NOEXCEPT { return 0; }

  SBE_NODISCARD bool orderIdInActingVersion() SBE_NOEXCEPT { return true; }

  SBE_NODISCARD static SBE_CONSTEXPR std::size_t orderIdEncodingOffset() SBE_NOEXCEPT { return 0; }

  static SBE_CONSTEXPR std::uint64_t orderIdNullValue() SBE_NOEXCEPT
  {
    return SBE_NULLVALUE_UINT64;
  }

  static SBE_CONSTEXPR std::uint64_t orderIdMinValue() SBE_NOEXCEPT { return UINT64_C(0x0); }

  static SBE_CONSTEXPR std::uint64_t orderIdMaxValue() SBE_NOEXCEPT
  {
    return UINT64_C(0xfffffffffffffffe);
  }

  static SBE_CONSTEXPR std::size_t orderIdEncodingLength() SBE_NOEXCEPT { return 8; }

  SBE_NODISCARD std::uint64_t orderId() const SBE_NOEXCEPT
  {
    std::uint64_t val;
    std::memcpy(&val, m_buffer + m_offset + 0, sizeof(std::uint64_t));
    return SBE_LITTLE_ENDIAN_ENCODE_64(val);
  }

  CancelOrder& orderId(std::uint64_t const value) SBE_NOEXCEPT
  {
    std::uint64_t val = SBE_LITTLE_ENDIAN_ENCODE_64(value);
    std::memcpy(m_buffer + m_offset + 0, &val, sizeof(std::uint64_t));
    return *this;
  }

  SBE_NODISCARD static const char* origOrderIdMetaAttribute(MetaAttribute const metaAttribute) SBE_NOEXCEPT
  {
    switch (metaAttribute)
    {
    case MetaAttribute::PRESENCE:
      return "required";
    default:
      return "";
    }
  }

  static SBE_CONSTEXPR std::uint16_t origOrderIdId() SBE_NOEXCEPT { return 2; }

  SBE_NODISCARD static SBE_CONSTEXPR std::uint64_t origOrderIdSinceVersion() SBE_NOEXCEPT
  {
    return 0;
  }

  SBE_NODISCARD bool origOrderIdInActingVersion() SBE_NOEXCEPT { return true; }

  SBE_NODISCARD static SBE_CONSTEXPR std::size_t origOrderIdEncodingOffset() SBE_NOEXCEPT
  {
    return 8;
  }

  static SBE_CONSTEXPR std::uint64_t origOrderIdNullValue() SBE_NOEXCEPT
  {
    return SBE_NULLVALUE_UINT64;
  }

  static SBE_CONSTEXPR std::uint64_t origOrderIdMinValue() SBE_NOEXCEPT { return UINT64_C(0x0); }

  static SBE_CONSTEXPR std::uint64_t origOrderIdMaxValue() SBE_NOEXCEPT
  {
    return UINT64_C(0xfffffffffffffffe);
  }

  static SBE_CONSTEXPR std::size_t origOrderIdEncodingLength() SBE_NOEXCEPT { return 8; }

  SBE_NODISCARD std::uint64_t origOrderId() const SBE_NOEXCEPT
  {
    std::uint64_t val;
    std::memcpy(&val, m_buffer + m_offset + 8, sizeof(std::uint64_t));
    return SBE_LITTLE_ENDIAN_ENCODE_64(val);
  }

  CancelOrder& origOrderId(std::uint64_t const value) SBE_NOEXCEPT
  {
    std::uint64_t val = SBE_LITTLE_ENDIAN_ENCODE_64(value);
    std::memcpy(m_buffer + m_offset + 8, &val, sizeof(std::uint64_t));
    return *this;
  }

  SBE_NODISCARD static const char* cancelQuantityMetaAttribute(MetaAttribute const metaAttribute) SBE_NOEXCEPT
  {
    switch (metaAttribute)
    {
    case MetaAttribute::PRESENCE:
      return "required";
    default:
      return "";
    }
  }

  static SBE_CONSTEXPR std::uint16_t cancelQuantityId() SBE_NOEXCEPT { return 3; }

  SBE_NODISCARD static SBE_CONSTEXPR std::uint64_t cancelQuantitySinceVersion() SBE_NOEXCEPT
  {
    return 0;
  }

  SBE_NODISCARD bool cancelQuantityInActingVersion() SBE_NOEXCEPT { return true; }

  SBE_NODISCARD static SBE_CONSTEXPR std::size_t cancelQuantityEncodingOffset() SBE_NOEXCEPT
  {
    return 16;
  }

  static SBE_CONSTEXPR std::uint32_t cancelQuantityNullValue() SBE_NOEXCEPT
  {
    return SBE_NULLVALUE_UINT32;
  }

  static SBE_CONSTEXPR std::uint32_t cancelQuantityMinValue() SBE_NOEXCEPT { return UINT32_C(0x0); }

  static SBE_CONSTEXPR std::uint32_t cancelQuantityMaxValue() SBE_NOEXCEPT
  {
    return UINT32_C(0xfffffffe);
  }

  static SBE_CONSTEXPR std::size_t cancelQuantityEncodingLength() SBE_NOEXCEPT { return 4; }

  SBE_NODISCARD std::uint32_t cancelQuantity() const SBE_NOEXCEPT
  {
    std::uint32_t val;
    std::memcpy(&val, m_buffer + m_offset + 16, sizeof(std::uint32_t));
    return SBE_LITTLE_ENDIAN_ENCODE_32(val);
  }

  CancelOrder& cancelQuantity(std::uint32_t const value) SBE_NOEXCEPT
  {
    std::uint32_t val = SBE_LITTLE_ENDIAN_ENCODE_32(value);
    std::memcpy(m_buffer + m_offset + 16, &val, sizeof(std::uint32_t));
    return *this;
  }

  SBE_NODISCARD static const char* reasonMetaAttribute(MetaAttribute const metaAttribute) SBE_NOEXCEPT
  {
    switch (metaAttribute)
    {
    case MetaAttribute::SEMANTIC_TYPE:
      return "String";
    case MetaAttribute::PRESENCE:
      return "required";
    default:
      return "";
    }
  }

  static char const* reasonCharacterEncoding() SBE_NOEXCEPT { return "ASCII"; }

  static SBE_CONSTEXPR std::uint64_t reasonSinceVersion() SBE_NOEXCEPT { return 0; }

  bool reasonInActingVersion() SBE_NOEXCEPT { return true; }

  static SBE_CONSTEXPR std::uint16_t reasonId() SBE_NOEXCEPT { return 4; }

  static SBE_CONSTEXPR std::uint64_t reasonHeaderLength() SBE_NOEXCEPT { return 1; }

  SBE_NODISCARD std::uint8_t reasonLength() const
  {
    std::uint8_t length;
    std::memcpy(&length, m_buffer + sbePosition(), sizeof(std::uint8_t));
    return (length);
  }

  std::uint64_t skipReason()
  {
    std::uint64_t lengthOfLengthField = 1;
    std::uint64_t lengthPosition = sbePosition();
    std::uint8_t lengthFieldValue;
    std::memcpy(&lengthFieldValue, m_buffer + lengthPosition, sizeof(std::uint8_t));
    std::uint64_t dataLength = (lengthFieldValue);
    sbePosition(lengthPosition + lengthOfLengthField + dataLength);
    return dataLength;
  }

  SBE_NODISCARD const char* reason()
  {
    std::uint8_t lengthFieldValue;
    std::memcpy(&lengthFieldValue, m_buffer + sbePosition(), sizeof(std::uint8_t));
    char const* fieldPtr = m_buffer + sbePosition() + 1;
    sbePosition(sbePosition() + 1 + (lengthFieldValue));
    return fieldPtr;
  }

  std::uint64_t getReason(char* dst, std::uint64_t const length)
  {
    std::uint64_t lengthOfLengthField = 1;
    std::uint64_t lengthPosition = sbePosition();
    sbePosition(lengthPosition + lengthOfLengthField);
    std::uint8_t lengthFieldValue;
    std::memcpy(&lengthFieldValue, m_buffer + lengthPosition, sizeof(std::uint8_t));
    std::uint64_t dataLength = (lengthFieldValue);
    std::uint64_t bytesToCopy = length < dataLength ? length : dataLength;
    std::uint64_t pos = sbePosition();
    sbePosition(pos + dataLength);
    std::memcpy(dst, m_buffer + pos, static_cast<std::size_t>(bytesToCopy));
    return bytesToCopy;
  }

  CancelOrder& putReason(char const* src, std::uint8_t const length)
  {
    std::uint64_t lengthOfLengthField = 1;
    std::uint64_t lengthPosition = sbePosition();
    std::uint8_t lengthFieldValue = (length);
    sbePosition(lengthPosition + lengthOfLengthField);
    std::memcpy(m_buffer + lengthPosition, &lengthFieldValue, sizeof(std::uint8_t));
    if (length != std::uint8_t(0))
    {
      std::uint64_t pos = sbePosition();
      sbePosition(pos + length);
      std::memcpy(m_buffer + pos, src, length);
    }
    return *this;
  }

  std::string getReasonAsString()
  {
    std::uint64_t lengthOfLengthField = 1;
    std::uint64_t lengthPosition = sbePosition();
    sbePosition(lengthPosition + lengthOfLengthField);
    std::uint8_t lengthFieldValue;
    std::memcpy(&lengthFieldValue, m_buffer + lengthPosition, sizeof(std::uint8_t));
    std::uint64_t dataLength = (lengthFieldValue);
    std::uint64_t pos = sbePosition();
    std::string const result(m_buffer + pos, dataLength);
    sbePosition(pos + dataLength);
    return result;
  }

  std::string getReasonAsJsonEscapedString()
  {
    std::ostringstream oss;
    std::string s = getReasonAsString();

    for (auto const c : s)
    {
      switch (c)
      {
      case '"':
        oss << "\\\"";
        break;
      case '\\':
        oss << "\\\\";
        break;
      case '\b':
        oss << "\\b";
        break;
      case '\f':
        oss << "\\f";
        break;
      case '\n':
        oss << "\\n";
        break;
      case '\r':
        oss << "\\r";
        break;
      case '\t':
        oss << "\\t";
        break;

      default:
        if ('\x00' <= c && c <= '\x1f')
        {
          oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)(c);
        }
        else
        {
          oss << c;
        }
      }
    }

    return oss.str();
  }

#if __cplusplus >= 201703L
  std::string_view getReasonAsStringView()
  {
    std::uint64_t lengthOfLengthField = 1;
    std::uint64_t lengthPosition = sbePosition();
    sbePosition(lengthPosition + lengthOfLengthField);
    std::uint8_t lengthFieldValue;
    std::memcpy(&lengthFieldValue, m_buffer + lengthPosition, sizeof(std::uint8_t));
    std::uint64_t dataLength = (lengthFieldValue);
    std::uint64_t pos = sbePosition();
    std::string_view const result(m_buffer + pos, dataLength);
    sbePosition(pos + dataLength);
    return result;
  }
#endif

  CancelOrder& putReason(const std::string& str)
  {
    if (str.length() > 40)
    {
      throw std::runtime_error("std::string too long for length type [E109]");
    }
    return putReason(str.data(), static_cast<std::uint8_t>(str.length()));
  }

#if __cplusplus >= 201703L
  CancelOrder& putReason(const std::string_view str)
  {
    if (str.length() > 40)
    {
      throw std::runtime_error("std::string too long for length type [E109]");
    }
    return putReason(str.data(), static_cast<std::uint8_t>(str.length()));
  }
#endif

  template <typename CharT, typename Traits>
  friend std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& builder,
                                                       const CancelOrder& _writer)
  {
    CancelOrder writer(_writer.m_buffer, _writer.m_offset, _writer.m_bufferLength,
                       _writer.m_actingBlockLength, _writer.m_actingVersion);

    builder << '{';
    builder << R"("Name": "CancelOrder", )";
    builder << R"("sbeTemplateId": )";
    builder << writer.sbeTemplateId();
    builder << ", ";

    builder << R"("orderId": )";
    builder << +writer.orderId();

    builder << ", ";
    builder << R"("origOrderId": )";
    builder << +writer.origOrderId();

    builder << ", ";
    builder << R"("cancelQuantity": )";
    builder << +writer.cancelQuantity();

    builder << ", ";
    builder << R"("reason": )";
    builder << '"' << writer.getReasonAsJsonEscapedString().c_str() << '"';

    builder << '}';

    return builder;
  }

  void skip() { skipReason(); }

  SBE_NODISCARD static SBE_CONSTEXPR bool isConstLength() SBE_NOEXCEPT { return false; }

  SBE_NODISCARD static std::size_t computeLength(std::size_t reasonLength = 0)
  {
#if defined(__GNUG__) && !defined(__clang__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wtype-limits"
#endif
    std::size_t length = sbeBlockLength();

    length += reasonHeaderLength();
    if (reasonLength > 40LL)
    {
      throw std::runtime_error("reasonLength too long for length type [E109]");
    }
    length += reasonLength;

    return length;
#if defined(__GNUG__) && !defined(__clang__)
  #pragma GCC diagnostic pop
#endif
  }
};
} // namespace sample
} // namespace sbe
#endif
