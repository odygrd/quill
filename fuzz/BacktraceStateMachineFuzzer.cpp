// BacktraceStateMachineFuzzer - State-machine fuzzer for backtrace logging
//
// Unlike data-plane fuzzers that feed random arguments through LOG macros,
// this fuzzer uses fuzz input to drive the *sequence of API operations*:
//
// - init_backtrace with various capacities
// - LOG_BACKTRACE to store messages in the ring buffer
// - Normal LOG_* at various levels (some may trigger automatic backtrace flush)
// - flush_backtrace for explicit manual flush
// - Reinitialising backtrace with a different capacity mid-stream
// - Interleaving all of the above in fuzz-driven order
//
// The goal is to find state corruption in the BacktraceStorage ring buffer,
// ordering bugs during flush, and edge cases around capacity changes.

#define FUZZER_LOG_FILENAME "backtrace_state_machine_fuzz.log"
#define FUZZER_IMMEDIATE_FLUSH_LIMIT 256
#include "FuzzerHelper.h"

#include "FuzzDataExtractor.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/FileSink.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>

namespace
{
std::atomic<uint32_t> g_bt_logger_counter{0};

// Operations the fuzzer can perform
enum class Op : uint8_t
{
  LogBacktrace = 0,
  LogInfo = 1,
  LogWarning = 2,
  LogError = 3,
  LogCritical = 4,
  FlushBacktrace = 5,
  ReinitBacktrace = 6,
  LogBacktraceBurst = 7,
  ChangeFlushLevel = 8,
  LogDynamic = 9,
  LogBacktraceVariousTypes = 10,
  LogvBacktrace = 11,
  Count = 12
};
} // namespace

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size)
{
  if (size < 8 || !g_logger)
  {
    return 0;
  }

  FuzzDataExtractor extractor{data, size};

  // Create a dedicated logger for this invocation so we have a clean backtrace state
  uint32_t id = g_bt_logger_counter.fetch_add(1);
  std::string logger_name = "bt_fuzz_" + std::to_string(id);

  auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
    FUZZER_LOG_FILENAME,
    []()
    {
      quill::FileSinkConfig cfg;
      cfg.set_open_mode('a');
      cfg.set_filename_append_option(quill::FilenameAppendOption::None);
      return cfg;
    }(),
    quill::FileEventNotifier{});

  quill::Logger* logger = quill::Frontend::create_or_get_logger(logger_name, std::move(file_sink));
  logger->set_log_level(quill::LogLevel::TraceL3);

  // Fuzz-driven initial backtrace capacity (1-64)
  uint32_t initial_capacity = (extractor.get_byte() % 64u) + 1u;

  // Fuzz-driven flush level: None means manual flush only, or a real level for auto-flush
  uint8_t flush_level_byte = extractor.get_byte();
  quill::LogLevel flush_level;
  switch (flush_level_byte % 5u)
  {
  case 0:
    flush_level = quill::LogLevel::None;
    break;
  case 1:
    flush_level = quill::LogLevel::Error;
    break;
  case 2:
    flush_level = quill::LogLevel::Critical;
    break;
  case 3:
    flush_level = quill::LogLevel::Warning;
    break;
  case 4:
    flush_level = quill::LogLevel::Notice;
    break;
  default:
    flush_level = quill::LogLevel::None;
    break;
  }

  logger->init_backtrace(initial_capacity, flush_level);

  // State machine: consume fuzz bytes as operation selectors
  while (extractor.remaining() > 2u)
  {
    auto op = static_cast<Op>(extractor.get_byte() % static_cast<uint8_t>(Op::Count));

    switch (op)
    {
    case Op::LogBacktrace:
    {
      int32_t val = extractor.get_int32();
      std::string msg = extractor.get_string(64);
      LOG_BACKTRACE(logger, "bt val={} msg={}", val, msg);
      break;
    }
    case Op::LogInfo:
    {
      int32_t val = extractor.get_int32();
      LOG_INFO(logger, "info val={}", val);
      break;
    }
    case Op::LogWarning:
    {
      std::string msg = extractor.get_string(32);
      LOG_WARNING(logger, "warn msg={}", msg);
      break;
    }
    case Op::LogError:
    {
      // This may trigger an automatic backtrace flush if flush_level <= Error
      int32_t val = extractor.get_int32();
      LOG_ERROR(logger, "error val={}", val);
      break;
    }
    case Op::LogCritical:
    {
      // This may trigger an automatic backtrace flush if flush_level <= Critical
      std::string msg = extractor.get_string(32);
      LOG_CRITICAL(logger, "critical msg={}", msg);
      break;
    }
    case Op::FlushBacktrace:
    {
      // Explicit manual flush of the backtrace ring buffer
      logger->flush_backtrace();
      break;
    }
    case Op::ReinitBacktrace:
    {
      // Reinitialise with a different capacity - this clears stored events on the backend
      uint32_t new_capacity = (extractor.get_byte() % 64u) + 1u;

      // Also possibly change the flush level
      quill::LogLevel new_flush_level;
      switch (extractor.get_byte() % 5u)
      {
      case 0:
        new_flush_level = quill::LogLevel::None;
        break;
      case 1:
        new_flush_level = quill::LogLevel::Error;
        break;
      case 2:
        new_flush_level = quill::LogLevel::Critical;
        break;
      case 3:
        new_flush_level = quill::LogLevel::Warning;
        break;
      case 4:
        new_flush_level = quill::LogLevel::Notice;
        break;
      default:
        new_flush_level = quill::LogLevel::None;
        break;
      }

      logger->init_backtrace(new_capacity, new_flush_level);
      break;
    }
    case Op::LogBacktraceBurst:
    {
      // Burst of backtrace messages to overflow the ring buffer
      uint32_t count = (extractor.get_byte() % 32u) + 1u;
      for (uint32_t i = 0; i < count && extractor.remaining() > 2u; ++i)
      {
        int32_t val = extractor.get_int32();
        LOG_BACKTRACE(logger, "bt_burst i={} val={}", i, val);
      }
      break;
    }
    case Op::ChangeFlushLevel:
    {
      // Change only the flush level via reinit with same or different capacity
      // The frontend stores _backtrace_flush_level atomically, but the capacity
      // change goes through the queue — interesting interleaving
      uint32_t cap = (extractor.get_byte() % 64u) + 1u;
      auto new_level = static_cast<quill::LogLevel>(extractor.get_byte() % 9u);
      logger->init_backtrace(cap, new_level);
      break;
    }
    case Op::LogDynamic:
    {
      // Dynamic log level — could trigger backtrace flush if level is high enough
      auto log_level = static_cast<quill::LogLevel>(extractor.get_byte() % 9u);
      int32_t val = extractor.get_int32();
      LOG_DYNAMIC(logger, log_level, "dynamic val={}", val);
      break;
    }
    case Op::LogBacktraceVariousTypes:
    {
      // Backtrace with various argument types to stress the TransitEvent copy path
      int32_t ival = extractor.get_int32();
      double dval = extractor.get_double();
      std::string sval = extractor.get_string(48);
      uint64_t uval = extractor.get_uint64();
      LOG_BACKTRACE(logger, "bt_types i={} d={} s={} u={}", ival, dval, sval, uval);
      break;
    }
    case Op::LogvBacktrace:
    {
      int32_t a = extractor.get_int32();
      double b = extractor.get_double();
      LOGV_BACKTRACE(logger, "logv_bt", a, b);
      break;
    }
    default:
      break;
    }
  }

  // Clean up: remove the logger we created
  quill::Frontend::remove_logger_blocking(logger);

  return 0;
}
