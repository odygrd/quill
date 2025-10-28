#define FUZZER_LOG_FILENAME "basic_types_fuzz.log"
#include "FuzzerHelper.h"

#include "quill/LogMacros.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

// Helper to extract data from fuzzer input
class FuzzDataExtractor
{
public:
  explicit FuzzDataExtractor(uint8_t const* data, size_t size)
    : _data(data), _size(size), _offset(0)
  {
  }

  bool has_data() const { return _offset < _size; }

  uint8_t get_byte()
  {
    if (_offset < _size)
      return _data[_offset++];
    return 0;
  }

  int8_t get_int8() { return static_cast<int8_t>(get_byte()); }

  uint16_t get_uint16()
  {
    uint16_t value = 0;
    if (_offset + sizeof(uint16_t) <= _size)
    {
      std::memcpy(&value, _data + _offset, sizeof(uint16_t));
      _offset += sizeof(uint16_t);
    }
    return value;
  }

  int16_t get_int16() { return static_cast<int16_t>(get_uint16()); }

  uint32_t get_uint32()
  {
    uint32_t value = 0;
    if (_offset + sizeof(uint32_t) <= _size)
    {
      std::memcpy(&value, _data + _offset, sizeof(uint32_t));
      _offset += sizeof(uint32_t);
    }
    return value;
  }

  int32_t get_int32() { return static_cast<int32_t>(get_uint32()); }

  uint64_t get_uint64()
  {
    uint64_t value = 0;
    if (_offset + sizeof(uint64_t) <= _size)
    {
      std::memcpy(&value, _data + _offset, sizeof(uint64_t));
      _offset += sizeof(uint64_t);
    }
    return value;
  }

  int64_t get_int64() { return static_cast<int64_t>(get_uint64()); }

  float get_float()
  {
    float value = 0.0f;
    if (_offset + sizeof(float) <= _size)
    {
      std::memcpy(&value, _data + _offset, sizeof(float));
      _offset += sizeof(float);
    }
    return value;
  }

  double get_double()
  {
    double value = 0.0;
    if (_offset + sizeof(double) <= _size)
    {
      std::memcpy(&value, _data + _offset, sizeof(double));
      _offset += sizeof(double);
    }
    return value;
  }

  long double get_long_double()
  {
    long double value = 0.0L;
    if (_offset + sizeof(long double) <= _size)
    {
      std::memcpy(&value, _data + _offset, sizeof(long double));
      _offset += sizeof(long double);
    }
    return value;
  }

  bool get_bool() { return get_byte() & 1; }

  char get_char() { return static_cast<char>(get_byte()); }

  std::string get_string(size_t max_len = 256)
  {
    if (_offset >= _size)
      return "";

    size_t len = get_byte() % (max_len + 1);
    std::string result;
    for (size_t i = 0; i < len && _offset < _size; ++i)
    {
      result.push_back(static_cast<char>(_data[_offset++]));
    }
    return result;
  }

  std::string_view get_string_view()
  {
    if (_offset >= _size)
      return "";

    size_t len = get_byte();
    if (_offset + len > _size)
      len = _size - _offset;

    std::string_view result(reinterpret_cast<char const*>(_data + _offset), len);
    _offset += len;
    return result;
  }

  // Get null-terminated C string from fuzzer data
  // NOTE: Returns pointer to internal buffer - valid until next call
  char const* get_c_string(size_t max_len = 256)
  {
    if (_offset >= _size)
    {
      _c_string_buffer[0] = '\0';
      return _c_string_buffer.data();
    }

    size_t len = get_byte() % (max_len + 1);
    size_t actual_len = 0;

    for (size_t i = 0; i < len && _offset < _size && actual_len < _c_string_buffer.size() - 1; ++i)
    {
      _c_string_buffer[actual_len++] = static_cast<char>(_data[_offset++]);
    }

    // Ensure null termination - required by quill for char const*
    _c_string_buffer[actual_len] = '\0';
    return _c_string_buffer.data();
  }

private:
  uint8_t const* _data;
  size_t _size;
  size_t _offset;
  std::array<char, 512> _c_string_buffer; // Buffer for null-terminated C strings
};

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size)
{
  if (size < 3 || !g_logger)
    return 0;

  FuzzDataExtractor extractor(data, size);
  uint8_t selector = extractor.get_byte();

  // Test different basic types based on selector
  switch (selector % 25)
  {
  case 0:
  {
    // Test std::string
    std::string str = extractor.get_string();
    LOG_INFO(g_logger, "String: {}", str);
    break;
  }
  case 1:
  {
    // Test char const* with fuzzer data (properly null-terminated)
    char const* cstr = extractor.get_c_string();
    LOG_INFO(g_logger, "C-string: {}", cstr);
    break;
  }
  case 2:
  {
    // Test std::string_view
    std::string str = extractor.get_string();
    std::string_view sv{str};
    LOG_INFO(g_logger, "String view: {}", sv);
    break;
  }
  case 3:
  {
    // Test int8_t
    int8_t val = extractor.get_int8();
    LOG_INFO(g_logger, "int8_t: {}", val);
    break;
  }
  case 4:
  {
    // Test uint8_t
    uint8_t val = extractor.get_byte();
    LOG_INFO(g_logger, "uint8_t: {}", val);
    break;
  }
  case 5:
  {
    // Test int16_t
    int16_t val = extractor.get_int16();
    LOG_INFO(g_logger, "int16_t: {}", val);
    break;
  }
  case 6:
  {
    // Test uint16_t
    uint16_t val = extractor.get_uint16();
    LOG_INFO(g_logger, "uint16_t: {}", val);
    break;
  }
  case 7:
  {
    // Test int32_t
    int32_t val = extractor.get_int32();
    LOG_INFO(g_logger, "int32_t: {}", val);
    break;
  }
  case 8:
  {
    // Test uint32_t
    uint32_t val = extractor.get_uint32();
    LOG_INFO(g_logger, "uint32_t: {}", val);
    break;
  }
  case 9:
  {
    // Test int64_t
    int64_t val = extractor.get_int64();
    LOG_INFO(g_logger, "int64_t: {}", val);
    break;
  }
  case 10:
  {
    // Test uint64_t
    uint64_t val = extractor.get_uint64();
    LOG_INFO(g_logger, "uint64_t: {}", val);
    break;
  }
  case 11:
  {
    // Test float
    float val = extractor.get_float();
    LOG_INFO(g_logger, "float: {}", val);
    break;
  }
  case 12:
  {
    // Test double
    double val = extractor.get_double();
    LOG_INFO(g_logger, "double: {}", val);
    break;
  }
  case 13:
  {
    // Test long double
    long double val = extractor.get_long_double();
    LOG_INFO(g_logger, "long double: {}", val);
    break;
  }
  case 14:
  {
    // Test bool
    bool val = extractor.get_bool();
    LOG_INFO(g_logger, "bool: {}", val);
    break;
  }
  case 15:
  {
    // Test char
    char val = extractor.get_char();
    LOG_INFO(g_logger, "char: {}", val);
    break;
  }
  case 16:
  {
    // Test mixed types
    int32_t i = extractor.get_int32();
    double d = extractor.get_double();
    std::string s = extractor.get_string(32);
    LOG_INFO(g_logger, "Mixed: {} {} {}", i, d, s);
    break;
  }
  case 17:
  {
    // Test format specifiers with int
    int32_t val = extractor.get_int32();
    LOG_INFO(g_logger, "Hex: {:x}, Oct: {:o}, Bin: {:b}", val, val, val);
    break;
  }
  case 18:
  {
    // Test format specifiers with double
    double val = extractor.get_double();
    LOG_INFO(g_logger, "Double formats: {:.2f}, {:.2e}, {:.2g}", val, val, val);
    break;
  }
  case 19:
  {
    // Test LOGV macros
    int32_t a = extractor.get_int32();
    double b = extractor.get_double();
    LOGV_INFO(g_logger, "LOGV test", a, b);
    break;
  }
  case 20:
  {
    // Test multiple log levels in sequence
    std::string msg = extractor.get_string(32);
    LOG_TRACE_L3(g_logger, "TraceL3: {}", msg);
    LOG_DEBUG(g_logger, "Debug: {}", msg);
    LOG_NOTICE(g_logger, "Notice: {}", msg);
    break;
  }
  case 21:
  {
    // Test multiple char const* in one log
    char const* str1 = extractor.get_c_string(32);
    char const* str2 = extractor.get_c_string(32);
    LOG_INFO(g_logger, "C-strings: {} {}", str1, str2);
    break;
  }
  case 22:
  {
    // Test signed/unsigned boundaries
    int64_t min_int = std::numeric_limits<int64_t>::min();
    uint64_t max_uint = std::numeric_limits<uint64_t>::max();
    LOG_INFO(g_logger, "Boundaries: {}, {}", min_int, max_uint);
    break;
  }
  case 23:
  {
    // Test special float values
    float inf = std::numeric_limits<float>::infinity();
    float neg_inf = -std::numeric_limits<float>::infinity();
    float nan_val = std::numeric_limits<float>::quiet_NaN();
    LOG_INFO(g_logger, "Special floats: {}, {}, {}", inf, neg_inf, nan_val);
    break;
  }
  case 24:
  {
    // Test empty string
    std::string empty;
    LOG_INFO(g_logger, "Empty string: '{}'", empty);
    break;
  }
  }

  return 0;
}
