/* Generated SBE (Simple Binary Encoding) message codec */
#ifndef _SBE_SAMPLE_NEWORDER_CXX_H_
#define _SBE_SAMPLE_NEWORDER_CXX_H_

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

class NewOrder
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
  static constexpr std::uint16_t SBE_BLOCK_LENGTH = static_cast<std::uint16_t>(17);
  static constexpr std::uint16_t SBE_TEMPLATE_ID = static_cast<std::uint16_t>(1);
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

  NewOrder() = default;

  NewOrder(char* buffer, std::uint64_t const offset, std::uint64_t const bufferLength,
           std::uint64_t const actingBlockLength, std::uint64_t const actingVersion)
    : m_buffer(buffer),
      m_bufferLength(bufferLength),
      m_offset(offset),
      m_position(sbeCheckPosition(offset + actingBlockLength)),
      m_actingBlockLength(actingBlockLength),
      m_actingVersion(actingVersion)
  {
  }

  NewOrder(char* buffer, std::uint64_t const bufferLength)
    : NewOrder(buffer, 0, bufferLength, sbeBlockLength(), sbeSchemaVersion())
  {
  }

  NewOrder(char* buffer, std::uint64_t const bufferLength, std::uint64_t const actingBlockLength,
           std::uint64_t const actingVersion)
    : NewOrder(buffer, 0, bufferLength, actingBlockLength, actingVersion)
  {
  }

  SBE_NODISCARD static SBE_CONSTEXPR std::uint16_t sbeBlockLength() SBE_NOEXCEPT
  {
    return static_cast<std::uint16_t>(17);
  }

  SBE_NODISCARD static SBE_CONSTEXPR std::uint64_t sbeBlockAndHeaderLength() SBE_NOEXCEPT
  {
    return messageHeader::encodedLength() + sbeBlockLength();
  }

  SBE_NODISCARD static SBE_CONSTEXPR std::uint16_t sbeTemplateId() SBE_NOEXCEPT
  {
    return static_cast<std::uint16_t>(1);
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

  NewOrder& wrapForEncode(char* buffer, std::uint64_t const offset, std::uint64_t const bufferLength)
  {
    m_buffer = buffer;
    m_bufferLength = bufferLength;
    m_offset = offset;
    m_actingBlockLength = sbeBlockLength();
    m_actingVersion = sbeSchemaVersion();
    m_position = sbeCheckPosition(m_offset + m_actingBlockLength);
    return *this;
  }

  NewOrder& wrapAndApplyHeader(char* buffer, std::uint64_t const offset, std::uint64_t const bufferLength)
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

  NewOrder& wrapForDecode(char* buffer, std::uint64_t const offset, std::uint64_t const actingBlockLength,
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

  NewOrder& sbeRewind()
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
    NewOrder skipper(m_buffer, m_offset, m_bufferLength, m_actingBlockLength, m_actingVersion);
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

  NewOrder& orderId(std::uint64_t const value) SBE_NOEXCEPT
  {
    std::uint64_t val = SBE_LITTLE_ENDIAN_ENCODE_64(value);
    std::memcpy(m_buffer + m_offset + 0, &val, sizeof(std::uint64_t));
    return *this;
  }

  SBE_NODISCARD static const char* priceMetaAttribute(MetaAttribute const metaAttribute) SBE_NOEXCEPT
  {
    switch (metaAttribute)
    {
    case MetaAttribute::PRESENCE:
      return "required";
    default:
      return "";
    }
  }

  static SBE_CONSTEXPR std::uint16_t priceId() SBE_NOEXCEPT { return 2; }

  SBE_NODISCARD static SBE_CONSTEXPR std::uint64_t priceSinceVersion() SBE_NOEXCEPT { return 0; }

  SBE_NODISCARD bool priceInActingVersion() SBE_NOEXCEPT { return true; }

  SBE_NODISCARD static SBE_CONSTEXPR std::size_t priceEncodingOffset() SBE_NOEXCEPT { return 8; }

  static SBE_CONSTEXPR std::uint32_t priceNullValue() SBE_NOEXCEPT { return SBE_NULLVALUE_UINT32; }

  static SBE_CONSTEXPR std::uint32_t priceMinValue() SBE_NOEXCEPT { return UINT32_C(0x0); }

  static SBE_CONSTEXPR std::uint32_t priceMaxValue() SBE_NOEXCEPT { return UINT32_C(0xfffffffe); }

  static SBE_CONSTEXPR std::size_t priceEncodingLength() SBE_NOEXCEPT { return 4; }

  SBE_NODISCARD std::uint32_t price() const SBE_NOEXCEPT
  {
    std::uint32_t val;
    std::memcpy(&val, m_buffer + m_offset + 8, sizeof(std::uint32_t));
    return SBE_LITTLE_ENDIAN_ENCODE_32(val);
  }

  NewOrder& price(std::uint32_t const value) SBE_NOEXCEPT
  {
    std::uint32_t val = SBE_LITTLE_ENDIAN_ENCODE_32(value);
    std::memcpy(m_buffer + m_offset + 8, &val, sizeof(std::uint32_t));
    return *this;
  }

  SBE_NODISCARD static const char* quantityMetaAttribute(MetaAttribute const metaAttribute) SBE_NOEXCEPT
  {
    switch (metaAttribute)
    {
    case MetaAttribute::PRESENCE:
      return "required";
    default:
      return "";
    }
  }

  static SBE_CONSTEXPR std::uint16_t quantityId() SBE_NOEXCEPT { return 3; }

  SBE_NODISCARD static SBE_CONSTEXPR std::uint64_t quantitySinceVersion() SBE_NOEXCEPT { return 0; }

  SBE_NODISCARD bool quantityInActingVersion() SBE_NOEXCEPT { return true; }

  SBE_NODISCARD static SBE_CONSTEXPR std::size_t quantityEncodingOffset() SBE_NOEXCEPT
  {
    return 12;
  }

  static SBE_CONSTEXPR std::uint32_t quantityNullValue() SBE_NOEXCEPT
  {
    return SBE_NULLVALUE_UINT32;
  }

  static SBE_CONSTEXPR std::uint32_t quantityMinValue() SBE_NOEXCEPT { return UINT32_C(0x0); }

  static SBE_CONSTEXPR std::uint32_t quantityMaxValue() SBE_NOEXCEPT
  {
    return UINT32_C(0xfffffffe);
  }

  static SBE_CONSTEXPR std::size_t quantityEncodingLength() SBE_NOEXCEPT { return 4; }

  SBE_NODISCARD std::uint32_t quantity() const SBE_NOEXCEPT
  {
    std::uint32_t val;
    std::memcpy(&val, m_buffer + m_offset + 12, sizeof(std::uint32_t));
    return SBE_LITTLE_ENDIAN_ENCODE_32(val);
  }

  NewOrder& quantity(std::uint32_t const value) SBE_NOEXCEPT
  {
    std::uint32_t val = SBE_LITTLE_ENDIAN_ENCODE_32(value);
    std::memcpy(m_buffer + m_offset + 12, &val, sizeof(std::uint32_t));
    return *this;
  }

  SBE_NODISCARD static const char* sideMetaAttribute(MetaAttribute const metaAttribute) SBE_NOEXCEPT
  {
    switch (metaAttribute)
    {
    case MetaAttribute::PRESENCE:
      return "required";
    default:
      return "";
    }
  }

  static SBE_CONSTEXPR std::uint16_t sideId() SBE_NOEXCEPT { return 4; }

  SBE_NODISCARD static SBE_CONSTEXPR std::uint64_t sideSinceVersion() SBE_NOEXCEPT { return 0; }

  SBE_NODISCARD bool sideInActingVersion() SBE_NOEXCEPT { return true; }

  SBE_NODISCARD static SBE_CONSTEXPR std::size_t sideEncodingOffset() SBE_NOEXCEPT { return 16; }

  SBE_NODISCARD static SBE_CONSTEXPR std::size_t sideEncodingLength() SBE_NOEXCEPT { return 1; }

  SBE_NODISCARD std::uint8_t sideRaw() const SBE_NOEXCEPT
  {
    std::uint8_t val;
    std::memcpy(&val, m_buffer + m_offset + 16, sizeof(std::uint8_t));
    return (val);
  }

  SBE_NODISCARD Side::Value side() const
  {
    std::uint8_t val;
    std::memcpy(&val, m_buffer + m_offset + 16, sizeof(std::uint8_t));
    return Side::get((val));
  }

  NewOrder& side(Side::Value const value) SBE_NOEXCEPT
  {
    std::uint8_t val = (value);
    std::memcpy(m_buffer + m_offset + 16, &val, sizeof(std::uint8_t));
    return *this;
  }

  SBE_NODISCARD static const char* symbolMetaAttribute(MetaAttribute const metaAttribute) SBE_NOEXCEPT
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

  static char const* symbolCharacterEncoding() SBE_NOEXCEPT { return "ASCII"; }

  static SBE_CONSTEXPR std::uint64_t symbolSinceVersion() SBE_NOEXCEPT { return 0; }

  bool symbolInActingVersion() SBE_NOEXCEPT { return true; }

  static SBE_CONSTEXPR std::uint16_t symbolId() SBE_NOEXCEPT { return 5; }

  static SBE_CONSTEXPR std::uint64_t symbolHeaderLength() SBE_NOEXCEPT { return 1; }

  SBE_NODISCARD std::uint8_t symbolLength() const
  {
    std::uint8_t length;
    std::memcpy(&length, m_buffer + sbePosition(), sizeof(std::uint8_t));
    return (length);
  }

  std::uint64_t skipSymbol()
  {
    std::uint64_t lengthOfLengthField = 1;
    std::uint64_t lengthPosition = sbePosition();
    std::uint8_t lengthFieldValue;
    std::memcpy(&lengthFieldValue, m_buffer + lengthPosition, sizeof(std::uint8_t));
    std::uint64_t dataLength = (lengthFieldValue);
    sbePosition(lengthPosition + lengthOfLengthField + dataLength);
    return dataLength;
  }

  SBE_NODISCARD const char* symbol()
  {
    std::uint8_t lengthFieldValue;
    std::memcpy(&lengthFieldValue, m_buffer + sbePosition(), sizeof(std::uint8_t));
    char const* fieldPtr = m_buffer + sbePosition() + 1;
    sbePosition(sbePosition() + 1 + (lengthFieldValue));
    return fieldPtr;
  }

  std::uint64_t getSymbol(char* dst, std::uint64_t const length)
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

  NewOrder& putSymbol(char const* src, std::uint8_t const length)
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

  std::string getSymbolAsString()
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

  std::string getSymbolAsJsonEscapedString()
  {
    std::ostringstream oss;
    std::string s = getSymbolAsString();

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
  std::string_view getSymbolAsStringView()
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

  NewOrder& putSymbol(const std::string& str)
  {
    if (str.length() > 40)
    {
      throw std::runtime_error("std::string too long for length type [E109]");
    }
    return putSymbol(str.data(), static_cast<std::uint8_t>(str.length()));
  }

#if __cplusplus >= 201703L
  NewOrder& putSymbol(const std::string_view str)
  {
    if (str.length() > 40)
    {
      throw std::runtime_error("std::string too long for length type [E109]");
    }
    return putSymbol(str.data(), static_cast<std::uint8_t>(str.length()));
  }
#endif

  template <typename CharT, typename Traits>
  friend std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& builder,
                                                       const NewOrder& _writer)
  {
    NewOrder writer(_writer.m_buffer, _writer.m_offset, _writer.m_bufferLength,
                    _writer.m_actingBlockLength, _writer.m_actingVersion);

    builder << '{';
    builder << R"("Name": "NewOrder", )";
    builder << R"("sbeTemplateId": )";
    builder << writer.sbeTemplateId();
    builder << ", ";

    builder << R"("orderId": )";
    builder << +writer.orderId();

    builder << ", ";
    builder << R"("price": )";
    builder << +writer.price();

    builder << ", ";
    builder << R"("quantity": )";
    builder << +writer.quantity();

    builder << ", ";
    builder << R"("side": )";
    builder << '"' << writer.side() << '"';

    builder << ", ";
    builder << R"("symbol": )";
    builder << '"' << writer.getSymbolAsJsonEscapedString().c_str() << '"';

    builder << '}';

    return builder;
  }

  void skip() { skipSymbol(); }

  SBE_NODISCARD static SBE_CONSTEXPR bool isConstLength() SBE_NOEXCEPT { return false; }

  SBE_NODISCARD static std::size_t computeLength(std::size_t symbolLength = 0)
  {
#if defined(__GNUG__) && !defined(__clang__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wtype-limits"
#endif
    std::size_t length = sbeBlockLength();

    length += symbolHeaderLength();
    if (symbolLength > 40LL)
    {
      throw std::runtime_error("symbolLength too long for length type [E109]");
    }
    length += symbolLength;

    return length;
#if defined(__GNUG__) && !defined(__clang__)
  #pragma GCC diagnostic pop
#endif
  }
};
} // namespace sample
} // namespace sbe
#endif
