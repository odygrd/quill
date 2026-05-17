// MultiSinkFilterFuzzer - Tests logging to multiple sinks with filters
//
// This is a state-machine fuzzer that exercises:
// - Logger writing to multiple sinks simultaneously (file + file + rotating)
// - Sink-level filters with fuzz-driven accept/reject decisions
// - Different sink types processing the same log message
// - Filter interactions with backtrace flush
// - Varying log levels against filters that selectively pass/block

#define FUZZER_LOG_FILENAME "multi_sink_filter_fuzz.log"
#define FUZZER_IMMEDIATE_FLUSH_LIMIT 128
#include "FuzzerHelper.h"

#include "FuzzDataExtractor.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/filters/Filter.h"
#include "quill/sinks/FileSink.h"
#include "quill/sinks/RotatingFileSink.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace
{
std::atomic<uint32_t> g_ms_counter{0};

// A filter whose threshold is set at construction from fuzz input
class ThresholdFilter : public quill::Filter
{
public:
  ThresholdFilter(std::string name, quill::LogLevel threshold)
    : quill::Filter(std::move(name)), _threshold(threshold)
  {
  }

  bool filter(quill::MacroMetadata const* /* log_metadata */, uint64_t /* log_timestamp */,
              std::string_view /* thread_id */, std::string_view /* thread_name */,
              std::string_view /* logger_name */, quill::LogLevel log_level,
              std::string_view /* log_message */, std::string_view /* log_statement */) noexcept override
  {
    return log_level >= _threshold;
  }

private:
  quill::LogLevel _threshold;
};

// A filter that accepts every Nth message (fuzz-driven modulus)
class ModuloFilter : public quill::Filter
{
public:
  ModuloFilter(std::string name, uint32_t modulo)
    : quill::Filter(std::move(name)), _modulo(modulo > 0 ? modulo : 1)
  {
  }

  bool filter(quill::MacroMetadata const* /* log_metadata */, uint64_t /* log_timestamp */,
              std::string_view /* thread_id */, std::string_view /* thread_name */,
              std::string_view /* logger_name */, quill::LogLevel /* log_level */,
              std::string_view /* log_message */, std::string_view /* log_statement */) noexcept override
  {
    return (_count++ % _modulo) == 0;
  }

private:
  uint32_t _modulo;
  uint32_t _count{0};
};
} // namespace

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size)
{
  if (size < 10 || !g_logger)
  {
    return 0;
  }

  FuzzDataExtractor extractor{data, size};

  uint32_t id = g_ms_counter.fetch_add(1);

  // Reuse a fixed set of file names across iterations. We create fresh sinks
  // each time so per-sink filters do not accumulate between fuzz inputs.
  auto file_sink = quill::Frontend::create_sink<quill::FileSink>(
    "multi_sink_a.log",
    []()
    {
      quill::FileSinkConfig cfg;
      cfg.set_open_mode('w');
      cfg.set_filename_append_option(quill::FilenameAppendOption::None);
      return cfg;
    }(),
    quill::FileEventNotifier{});

  auto threshold = static_cast<quill::LogLevel>(extractor.get_byte() % 9u);
  file_sink->add_filter(std::make_unique<ThresholdFilter>("threshold_" + std::to_string(id), threshold));

  // --- Create sink 2: second file sink with a modulo filter ---
  auto file_sink_b = quill::Frontend::create_sink<quill::FileSink>(
    "multi_sink_b.log",
    []()
    {
      quill::FileSinkConfig cfg;
      cfg.set_open_mode('w');
      cfg.set_filename_append_option(quill::FilenameAppendOption::None);
      return cfg;
    }(),
    quill::FileEventNotifier{});

  uint32_t modulo = (extractor.get_byte() % 7u) + 1u;
  file_sink_b->add_filter(std::make_unique<ModuloFilter>("modulo_" + std::to_string(id), modulo));

  // --- Create sink 3: rotating file sink with small rotation, no filter ---
  quill::RotatingFileSinkConfig rot_cfg;
  rot_cfg.set_open_mode('w');
  rot_cfg.set_filename_append_option(quill::FilenameAppendOption::None);
  rot_cfg.set_rotation_max_file_size((extractor.get_byte() % 4u + 1u) * 1024u);
  rot_cfg.set_max_backup_files(extractor.get_byte() % 3u + 1u);

  auto rotating_sink =
    quill::Frontend::create_sink<quill::RotatingFileSink>("multi_sink_rot.log", std::move(rot_cfg));

  // --- Create logger with all three sinks ---
  std::string logger_name = "multi_sink_fuzz_" + std::to_string(id);

  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    logger_name, {std::move(file_sink), std::move(file_sink_b), std::move(rotating_sink)});

  logger->set_log_level(quill::LogLevel::TraceL3);

  // Optionally enable backtrace on this multi-sink logger
  bool use_backtrace = extractor.get_bool();
  if (use_backtrace)
  {
    uint32_t bt_capacity = (extractor.get_byte() % 16u) + 1u;
    auto bt_flush_level = static_cast<quill::LogLevel>(extractor.get_byte() % 9u);
    logger->init_backtrace(bt_capacity, bt_flush_level);
  }

  // --- State machine: drive operations from fuzz data ---
  while (extractor.remaining() > 2u)
  {
    uint8_t op = extractor.get_byte() % 10u;

    switch (op)
    {
    case 0:
    {
      // LOG_INFO — may or may not pass threshold filter on sink 1
      std::string msg = extractor.get_string(48);
      int32_t val = extractor.get_int32();
      LOG_INFO(logger, "multi info {} {}", msg, val);
      break;
    }
    case 1:
    {
      LOG_DEBUG(logger, "multi debug {}", extractor.get_int32());
      break;
    }
    case 2:
    {
      LOG_WARNING(logger, "multi warn {}", extractor.get_string(32));
      break;
    }
    case 3:
    {
      // May trigger backtrace flush depending on flush level
      LOG_ERROR(logger, "multi error {}", extractor.get_int32());
      break;
    }
    case 4:
    {
      LOG_CRITICAL(logger, "multi critical {}", extractor.get_string(32));
      break;
    }
    case 5:
    {
      // Dynamic log level
      auto log_level = static_cast<quill::LogLevel>(extractor.get_byte() % 9u);
      LOG_DYNAMIC(logger, log_level, "multi dynamic {} {}", extractor.get_int32(), extractor.get_string(24));
      break;
    }
    case 6:
    {
      if (use_backtrace)
      {
        LOG_BACKTRACE(logger, "multi bt {} {}", extractor.get_int32(), extractor.get_string(32));
      }
      else
      {
        LOG_TRACE_L1(logger, "multi trace {}", extractor.get_int32());
      }
      break;
    }
    case 7:
    {
      if (use_backtrace)
      {
        logger->flush_backtrace();
      }
      break;
    }
    case 8:
    {
      // Burst of messages to stress filter + rotation interaction
      uint32_t count = (extractor.get_byte() % 16u) + 1u;
      for (uint32_t i = 0; i < count && extractor.remaining() > 2u; ++i)
      {
        LOG_INFO(logger, "multi burst {} {}", i, extractor.get_int32());
      }
      break;
    }
    case 9:
    {
      // Multiple argument types
      int32_t iv = extractor.get_int32();
      double dv = extractor.get_double();
      std::string sv = extractor.get_string(24);
      uint64_t uv = extractor.get_uint64();
      LOG_INFO(logger, "multi types {} {} {} {}", iv, dv, sv, uv);
      break;
    }
    default:
      break;
    }
  }

  quill::Frontend::remove_logger_blocking(logger);

  return 0;
}
