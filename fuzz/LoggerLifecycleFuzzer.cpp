// LoggerLifecycleFuzzer - Tests logger creation, usage, and removal under fuzzer-driven timing
//
// This fuzzer exercises the logger lifecycle path:
// - Creating multiple loggers with unique names
// - Logging through different loggers concurrently with the global one
// - Removing loggers while the backend is running
// - Logger log level changes
// - Ensuring removed loggers are not reused

#define FUZZER_LOG_FILENAME "logger_lifecycle_fuzz.log"
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
#include <vector>

static std::atomic<uint32_t> g_logger_counter{0};

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size)
{
  if (size < 5 || !g_logger)
    return 0;

  FuzzDataExtractor extractor(data, size);
  uint8_t selector = extractor.get_byte();

  switch (selector % 6)
  {
  case 0:
  {
    // Create a logger, log through it, remove it
    uint32_t id = g_logger_counter.fetch_add(1);
    std::string logger_name = "fuzz_logger_" + std::to_string(id);

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

    // Log a few messages
    uint32_t count = extractor.get_byte() % 10 + 1;
    for (uint32_t i = 0; i < count && extractor.has_data(); ++i)
    {
      int32_t val = extractor.get_int32();
      LOG_INFO(logger, "Logger {} msg {}: {}", logger_name, i, val);
    }

    // Remove the logger
    quill::Frontend::remove_logger_blocking(logger);
    break;
  }
  case 1:
  {
    // Create multiple loggers, log through all, remove all
    uint32_t base_id = g_logger_counter.fetch_add(3);
    uint32_t num_loggers = (extractor.get_byte() % 3) + 1;

    std::vector<quill::Logger*> loggers;
    loggers.reserve(num_loggers);

    for (uint32_t i = 0; i < num_loggers; ++i)
    {
      std::string logger_name = "fuzz_multi_" + std::to_string(base_id + i);

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
      loggers.push_back(logger);
    }

    // Log through all loggers in round-robin
    uint32_t msg_count = extractor.get_byte() % 15 + 1;
    for (uint32_t i = 0; i < msg_count && extractor.has_data(); ++i)
    {
      quill::Logger* logger = loggers[i % loggers.size()];
      int32_t val = extractor.get_int32();
      LOG_INFO(logger, "Multi-logger msg {}: {}", i, val);
    }

    // Remove all loggers
    for (auto* logger : loggers)
    {
      quill::Frontend::remove_logger_blocking(logger);
    }
    break;
  }
  case 2:
  {
    // Create logger, change log level, log at various levels, remove
    uint32_t id = g_logger_counter.fetch_add(1);
    std::string logger_name = "fuzz_level_" + std::to_string(id);

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

    // Set log level from fuzzer data
    auto log_level = static_cast<quill::LogLevel>(extractor.get_byte() % 9);
    logger->set_log_level(log_level);

    // Log at various levels - some will be filtered
    LOG_TRACE_L3(logger, "TraceL3 from {}", logger_name);
    LOG_DEBUG(logger, "Debug from {}", logger_name);
    LOG_INFO(logger, "Info from {}", logger_name);
    LOG_WARNING(logger, "Warning from {}", logger_name);
    LOG_ERROR(logger, "Error from {}", logger_name);

    // Change log level mid-stream
    auto new_level = static_cast<quill::LogLevel>(extractor.get_byte() % 9);
    logger->set_log_level(new_level);

    LOG_INFO(logger, "After level change to {}", static_cast<int>(new_level));

    quill::Frontend::remove_logger_blocking(logger);
    break;
  }
  case 3:
  {
    // Create and remove logger rapidly (lifecycle churn)
    uint32_t iterations = extractor.get_byte() % 5 + 1;
    for (uint32_t i = 0; i < iterations; ++i)
    {
      uint32_t id = g_logger_counter.fetch_add(1);
      std::string logger_name = "fuzz_churn_" + std::to_string(id);

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

      LOG_INFO(logger, "Churn logger {} iteration {}", logger_name, i);

      quill::Frontend::remove_logger_blocking(logger);
    }
    break;
  }
  case 4:
  {
    // Log through both global logger and a temporary logger simultaneously
    uint32_t id = g_logger_counter.fetch_add(1);
    std::string logger_name = "fuzz_dual_" + std::to_string(id);

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

    quill::Logger* temp_logger = quill::Frontend::create_or_get_logger(logger_name, std::move(file_sink));
    temp_logger->set_log_level(quill::LogLevel::TraceL3);

    uint32_t count = extractor.get_byte() % 10 + 1;
    for (uint32_t i = 0; i < count && extractor.has_data(); ++i)
    {
      int32_t val = extractor.get_int32();
      // Alternate between global and temp logger
      if (i & 1)
      {
        LOG_INFO(g_logger, "Global msg {}: {}", i, val);
      }
      else
      {
        LOG_INFO(temp_logger, "Temp msg {}: {}", i, val);
      }
    }

    quill::Frontend::remove_logger_blocking(temp_logger);
    break;
  }
  case 5:
  {
    // Create logger with same name as existing (get_or_create semantics)
    uint32_t id = g_logger_counter.fetch_add(1);
    std::string logger_name = "fuzz_reuse_" + std::to_string(id);

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

    // Create logger
    quill::Logger* logger1 = quill::Frontend::create_or_get_logger(logger_name, std::move(file_sink));
    logger1->set_log_level(quill::LogLevel::TraceL3);
    LOG_INFO(logger1, "First creation of {}", logger_name);

    // Get same logger again (should return same pointer)
    quill::Logger* logger2 = quill::Frontend::create_or_get_logger(logger_name);
    LOG_INFO(logger2, "Got same logger {}", logger_name);

    // Only remove once
    quill::Frontend::remove_logger_blocking(logger1);
    break;
  }
  }

  return 0;
}
