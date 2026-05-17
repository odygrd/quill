// MultithreadFuzzer - Stress tests concurrent logging from multiple threads
//
// This fuzzer spawns multiple threads that log concurrently

#define FUZZER_LOG_FILENAME "multithread_fuzz.log"
#include "FuzzerHelper.h"

#include "FuzzDataExtractor.h"
#include "quill/LogMacros.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

static void worker_thread(uint8_t const* data, size_t size, uint8_t thread_id)
{
  FuzzDataExtractor extractor(data, size);

  uint8_t strategy = extractor.get_byte();

  switch (strategy % 6)
  {
  case 0:
  {
    // Rapid-fire small messages
    uint32_t count = extractor.get_byte() % 20 + 1;
    for (uint32_t i = 0; i < count && extractor.has_data(); ++i)
    {
      int32_t val = extractor.get_int32();
      LOG_INFO(g_logger, "T{} rapid {}: {}", thread_id, i, val);
    }
    break;
  }
  case 1:
  {
    // Variable-sized string messages
    uint32_t count = extractor.get_byte() % 10 + 1;
    for (uint32_t i = 0; i < count && extractor.has_data(); ++i)
    {
      std::string msg = extractor.get_string(64);
      LOG_INFO(g_logger, "T{} str: {}", thread_id, msg);
    }
    break;
  }
  case 2:
  {
    // Mixed log levels
    while (extractor.remaining() > 4)
    {
      uint32_t val = extractor.get_uint32();
      uint8_t level = extractor.get_byte() % 5;
      switch (level)
      {
      case 0:
        LOG_TRACE_L1(g_logger, "T{} trace: {}", thread_id, val);
        break;
      case 1:
        LOG_DEBUG(g_logger, "T{} debug: {}", thread_id, val);
        break;
      case 2:
        LOG_INFO(g_logger, "T{} info: {}", thread_id, val);
        break;
      case 3:
        LOG_WARNING(g_logger, "T{} warn: {}", thread_id, val);
        break;
      case 4:
        LOG_ERROR(g_logger, "T{} error: {}", thread_id, val);
        break;
      }
    }
    break;
  }
  case 3:
  {
    // Multiple arguments
    while (extractor.remaining() > 16)
    {
      int32_t a = extractor.get_int32();
      double b = extractor.get_double();
      std::string c = extractor.get_string(16);
      LOG_INFO(g_logger, "T{} multi: {} {} {}", thread_id, a, b, c);
    }
    break;
  }
  case 4:
  {
    // LOGV macros
    while (extractor.remaining() > 8)
    {
      uint32_t x = extractor.get_uint32();
      uint32_t y = extractor.get_uint32();
      LOGV_INFO(g_logger, "LOGV from thread", thread_id, x, y);
    }
    break;
  }
  case 5:
  {
    // Large messages to stress queue growth
    uint32_t count = extractor.get_byte() % 5 + 1;
    for (uint32_t i = 0; i < count && extractor.remaining() > 4; ++i)
    {
      std::string msg = extractor.get_bytes(extractor.get_byte() % 200 + 50);
      LOG_INFO(g_logger, "T{} large {}: {}", thread_id, i, msg);
    }
    break;
  }
  }
}

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size)
{
  if (size < 10 || !g_logger)
    return 0;

  FuzzDataExtractor extractor(data, size);

  // First byte: number of threads (2-4)
  uint8_t num_threads = (extractor.get_byte() % 3) + 2;

  // Split remaining data among threads
  size_t remaining = extractor.remaining();
  size_t per_thread = remaining / num_threads;

  if (per_thread < 4)
    return 0;

  std::vector<std::thread> threads;
  threads.reserve(num_threads);

  size_t offset = size - remaining; // current offset into data

  for (uint8_t t = 0; t < num_threads; ++t)
  {
    size_t thread_size = (t == num_threads - 1) ? (size - offset) : per_thread;
    threads.emplace_back(worker_thread, data + offset, thread_size, t);
    offset += per_thread;
  }

  for (auto& t : threads)
  {
    t.join();
  }

  return 0;
}
