// DynamicLogLevelFuzzer - Tests LOG_DYNAMIC macro with runtime log level selection
//
// This fuzzer exercises the runtime metadata path which differs from the compile-time
// macro path. It stresses:
// - Runtime log level dispatch
// - Out-of-range log level values (mapped to valid range)
// - LOGV_DYNAMIC and LOGJ_DYNAMIC variants
// - Mixing dynamic and static log calls
// - should_log_statement() checks under fuzzer-driven levels

#define FUZZER_LOG_FILENAME "dynamic_log_level_fuzz.log"
#include "FuzzerHelper.h"

#include "FuzzDataExtractor.h"
#include "quill/LogMacros.h"
#include "quill/core/LogLevel.h"

#include <cstddef>
#include <cstdint>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size)
{
  if (size < 3 || !g_logger)
    return 0;

  FuzzDataExtractor extractor(data, size);
  uint8_t selector = extractor.get_byte();

  switch (selector % 8)
  {
  case 0:
  {
    // Test LOG_DYNAMIC with all valid log levels
    uint8_t level_byte = extractor.get_byte();
    // Map to valid LogLevel values (TraceL3=0 through Critical=8, skip Backtrace/None)
    auto log_level = static_cast<quill::LogLevel>(level_byte % 9);
    int32_t val = extractor.get_int32();
    LOG_DYNAMIC(g_logger, log_level, "Dynamic level {}: val={}", static_cast<int>(log_level), val);
    break;
  }
  case 1:
  {
    // Test LOGV_DYNAMIC
    uint8_t level_byte = extractor.get_byte();
    auto log_level = static_cast<quill::LogLevel>(level_byte % 9);
    int32_t a = extractor.get_int32();
    double b = extractor.get_double();
    LOGV_DYNAMIC(g_logger, log_level, "LOGV dynamic", a, b);
    break;
  }
  case 2:
  {
    // Test LOGJ_DYNAMIC
    uint8_t level_byte = extractor.get_byte();
    auto log_level = static_cast<quill::LogLevel>(level_byte % 9);
    int32_t x = extractor.get_int32();
    std::string y = extractor.get_string(32);
    LOGJ_DYNAMIC(g_logger, log_level, "LOGJ dynamic", x, y);
    break;
  }
  case 3:
  {
    // Rapid level changes - different level each message
    while (extractor.remaining() > 4)
    {
      auto log_level = static_cast<quill::LogLevel>(extractor.get_byte() % 9);
      int32_t val = extractor.get_int32();
      LOG_DYNAMIC(g_logger, log_level, "Rapid: {}", val);
    }
    break;
  }
  case 4:
  {
    // Mix dynamic and static log calls
    while (extractor.remaining() > 8)
    {
      auto log_level = static_cast<quill::LogLevel>(extractor.get_byte() % 9);
      int32_t val = extractor.get_int32();

      if (extractor.get_byte() & 1)
      {
        LOG_DYNAMIC(g_logger, log_level, "Dynamic: {}", val);
      }
      else
      {
        LOG_INFO(g_logger, "Static: {}", val);
      }
    }
    break;
  }
  case 5:
  {
    // Test with string arguments at different levels
    while (extractor.remaining() > 4)
    {
      auto log_level = static_cast<quill::LogLevel>(extractor.get_byte() % 9);
      std::string msg = extractor.get_string(64);
      LOG_DYNAMIC(g_logger, log_level, "Msg: {}", msg);
    }
    break;
  }
  case 6:
  {
    // Test with multiple arguments
    if (extractor.remaining() > 20)
    {
      auto log_level = static_cast<quill::LogLevel>(extractor.get_byte() % 9);
      int32_t a = extractor.get_int32();
      double b = extractor.get_double();
      std::string c = extractor.get_string(16);
      uint64_t d = extractor.get_uint64();
      LOG_DYNAMIC(g_logger, log_level, "Multi: {} {} {} {}", a, b, c, d);
    }
    break;
  }
  case 7:
  {
    // Test all levels sequentially with same data
    int32_t val = extractor.get_int32();
    std::string msg = extractor.get_string(16);

    LOG_DYNAMIC(g_logger, quill::LogLevel::TraceL3, "L3: {} {}", val, msg);
    LOG_DYNAMIC(g_logger, quill::LogLevel::TraceL2, "L2: {} {}", val, msg);
    LOG_DYNAMIC(g_logger, quill::LogLevel::TraceL1, "L1: {} {}", val, msg);
    LOG_DYNAMIC(g_logger, quill::LogLevel::Debug, "Dbg: {} {}", val, msg);
    LOG_DYNAMIC(g_logger, quill::LogLevel::Info, "Inf: {} {}", val, msg);
    LOG_DYNAMIC(g_logger, quill::LogLevel::Notice, "Not: {} {}", val, msg);
    LOG_DYNAMIC(g_logger, quill::LogLevel::Warning, "Wrn: {} {}", val, msg);
    LOG_DYNAMIC(g_logger, quill::LogLevel::Error, "Err: {} {}", val, msg);
    LOG_DYNAMIC(g_logger, quill::LogLevel::Critical, "Crt: {} {}", val, msg);
    break;
  }
  }

  return 0;
}
