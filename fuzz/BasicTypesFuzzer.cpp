#define FUZZER_LOG_FILENAME "basic_types_fuzz.log"
#include "FuzzerHelper.h"

#include "FuzzDataExtractor.h"
#include "quill/LogMacros.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

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
