// RotatingSinkFuzzer - Tests RotatingFileSink with fuzzer-driven rotation parameters
//
// This fuzzer exercises the file rotation logic:
// - Small rotation sizes to trigger frequent rotations
// - Variable message sizes crossing rotation boundaries
// - Multiple rotation policies (size-based)
// - Max backup count limits
// - Rotation during active logging

#define FUZZER_LOG_FILENAME "rotating_fuzz.log"
#include "FuzzerHelper.h"

#include "FuzzDataExtractor.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/RotatingFileSink.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>

static std::atomic<uint32_t> g_rotating_counter{0};

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size)
{
  if (size < 5 || !g_logger)
    return 0;

  FuzzDataExtractor extractor(data, size);
  uint8_t selector = extractor.get_byte();

  // Each invocation creates a unique filename to avoid conflicts
  uint32_t id = g_rotating_counter.fetch_add(1);

  switch (selector % 5)
  {
  case 0:
  {
    // Small rotation size (1KB-4KB) to trigger frequent rotations
    std::string filename = "rot_small_" + std::to_string(id) + ".log";

    quill::RotatingFileSinkConfig cfg;
    cfg.set_open_mode('w');
    cfg.set_filename_append_option(quill::FilenameAppendOption::None);
    size_t rotation_size = (extractor.get_byte() % 4 + 1) * 1024; // 1KB-4KB
    cfg.set_rotation_max_file_size(rotation_size);
    cfg.set_max_backup_files(extractor.get_byte() % 5 + 1); // 1-5 backups

    auto sink = quill::Frontend::create_or_get_sink<quill::RotatingFileSink>(filename, std::move(cfg));

    std::string logger_name = "rot_small_" + std::to_string(id);
    quill::Logger* logger = quill::Frontend::create_or_get_logger(logger_name, std::move(sink));
    logger->set_log_level(quill::LogLevel::TraceL3);

    // Log enough data to trigger multiple rotations
    uint32_t count = extractor.get_byte() % 30 + 10;
    for (uint32_t i = 0; i < count && extractor.has_data(); ++i)
    {
      std::string msg = extractor.get_string(64);
      LOG_INFO(logger, "Rot {}: {}", i, msg);
    }

    quill::Frontend::remove_logger_blocking(logger);
    break;
  }
  case 1:
  {
    // Messages that are large relative to rotation size
    std::string filename = "rot_large_msg_" + std::to_string(id) + ".log";

    quill::RotatingFileSinkConfig cfg;
    cfg.set_open_mode('w');
    cfg.set_filename_append_option(quill::FilenameAppendOption::None);
    cfg.set_rotation_max_file_size(2048); // 2KB rotation
    cfg.set_max_backup_files(3);

    auto sink = quill::Frontend::create_or_get_sink<quill::RotatingFileSink>(filename, std::move(cfg));

    std::string logger_name = "rot_large_msg_" + std::to_string(id);
    quill::Logger* logger = quill::Frontend::create_or_get_logger(logger_name, std::move(sink));
    logger->set_log_level(quill::LogLevel::TraceL3);

    // Log messages that are near or exceed rotation size
    uint32_t count = extractor.get_byte() % 10 + 5;
    for (uint32_t i = 0; i < count && extractor.remaining() > 4; ++i)
    {
      // Variable sizes: some small, some near rotation boundary
      size_t msg_len = extractor.get_byte() % 3 == 0 ? (extractor.get_byte() % 200 + 1800) // near 2KB
                                                     : (extractor.get_byte() % 100 + 1);   // small
      std::string msg = extractor.get_bytes(msg_len);
      LOG_INFO(logger, "LargeRot {}: {}", i, msg);
    }

    quill::Frontend::remove_logger_blocking(logger);
    break;
  }
  case 2:
  {
    // Mixed log levels with rotation
    std::string filename = "rot_levels_" + std::to_string(id) + ".log";

    quill::RotatingFileSinkConfig cfg;
    cfg.set_open_mode('w');
    cfg.set_filename_append_option(quill::FilenameAppendOption::None);
    cfg.set_rotation_max_file_size(4096); // 4KB rotation
    cfg.set_max_backup_files(2);

    auto sink = quill::Frontend::create_or_get_sink<quill::RotatingFileSink>(filename, std::move(cfg));

    std::string logger_name = "rot_levels_" + std::to_string(id);
    quill::Logger* logger = quill::Frontend::create_or_get_logger(logger_name, std::move(sink));
    logger->set_log_level(quill::LogLevel::TraceL3);

    while (extractor.remaining() > 4)
    {
      std::string msg = extractor.get_string(32);
      uint8_t level = extractor.get_byte() % 5;
      switch (level)
      {
      case 0:
        LOG_TRACE_L1(logger, "Trace: {}", msg);
        break;
      case 1:
        LOG_DEBUG(logger, "Debug: {}", msg);
        break;
      case 2:
        LOG_INFO(logger, "Info: {}", msg);
        break;
      case 3:
        LOG_WARNING(logger, "Warning: {}", msg);
        break;
      case 4:
        LOG_ERROR(logger, "Error: {}", msg);
        break;
      }
    }

    quill::Frontend::remove_logger_blocking(logger);
    break;
  }
  case 3:
  {
    // Max backup files = 0 (no backups, just overwrite)
    std::string filename = "rot_nobackup_" + std::to_string(id) + ".log";

    quill::RotatingFileSinkConfig cfg;
    cfg.set_open_mode('w');
    cfg.set_filename_append_option(quill::FilenameAppendOption::None);
    cfg.set_rotation_max_file_size(1024); // 1KB rotation
    cfg.set_max_backup_files(0);          // no backups

    auto sink = quill::Frontend::create_or_get_sink<quill::RotatingFileSink>(filename, std::move(cfg));

    std::string logger_name = "rot_nobackup_" + std::to_string(id);
    quill::Logger* logger = quill::Frontend::create_or_get_logger(logger_name, std::move(sink));
    logger->set_log_level(quill::LogLevel::TraceL3);

    uint32_t count = extractor.get_byte() % 20 + 5;
    for (uint32_t i = 0; i < count && extractor.has_data(); ++i)
    {
      std::string msg = extractor.get_string(32);
      LOG_INFO(logger, "NoBackup {}: {}", i, msg);
    }

    quill::Frontend::remove_logger_blocking(logger);
    break;
  }
  case 4:
  {
    // Rapid rotation with many small messages
    std::string filename = "rot_rapid_" + std::to_string(id) + ".log";

    quill::RotatingFileSinkConfig cfg;
    cfg.set_open_mode('w');
    cfg.set_filename_append_option(quill::FilenameAppendOption::None);
    cfg.set_rotation_max_file_size(512); // Very small: 512 bytes
    cfg.set_max_backup_files(10);

    auto sink = quill::Frontend::create_or_get_sink<quill::RotatingFileSink>(filename, std::move(cfg));

    std::string logger_name = "rot_rapid_" + std::to_string(id);
    quill::Logger* logger = quill::Frontend::create_or_get_logger(logger_name, std::move(sink));
    logger->set_log_level(quill::LogLevel::TraceL3);

    // Many small messages to trigger lots of rotations
    uint32_t count = extractor.get_byte() % 50 + 20;
    for (uint32_t i = 0; i < count && extractor.has_data(); ++i)
    {
      int32_t val = extractor.get_int32();
      LOG_INFO(logger, "Rapid {}: {}", i, val);
    }

    quill::Frontend::remove_logger_blocking(logger);
    break;
  }
  }

  return 0;
}
