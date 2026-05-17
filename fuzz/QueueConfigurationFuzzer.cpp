#include "FuzzDataExtractor.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/FileSink.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace
{
struct QueueFrontendOptions : quill::FrontendOptions
{
  static constexpr quill::QueueType queue_type = quill::QueueType::UnboundedDropping;
  static constexpr size_t initial_queue_capacity = 256;
  static constexpr size_t unbounded_queue_max_capacity = 4096;
};

using QueueFrontend = quill::FrontendImpl<QueueFrontendOptions>;
using QueueLogger = quill::LoggerImpl<QueueFrontendOptions>;

QueueLogger* g_logger{nullptr};
bool g_initialized{false};

void log_burst(FuzzDataExtractor& extractor, uint32_t iterations)
{
  for (uint32_t i = 0; i < iterations; ++i)
  {
    std::string text = extractor.get_bytes(static_cast<size_t>(extractor.get_byte() % 256u));
    std::string c_string_storage = extractor.get_string(96);
    char const* c_string = c_string_storage.c_str();
    int32_t iv = extractor.get_int32();
    uint64_t uv = extractor.get_uint64();
    double dv = extractor.get_double();

    switch (extractor.get_byte() % 4u)
    {
    case 0:
      LOG_INFO(g_logger, "queue i={} text={} cstr={} iv={} uv={} dv={}", i, text, c_string, iv, uv, dv);
      break;
    case 1:
      LOG_INFO(g_logger, "queue_named {text} {iv} {uv} {dv}", text, iv, uv, dv);
      break;
    case 2:
      LOGV_INFO(g_logger, "queue_logv", i, text, iv, uv, dv);
      break;
    case 3:
      LOG_DYNAMIC(g_logger, static_cast<quill::LogLevel>(extractor.get_byte() % 9u),
                  "queue_dynamic {} {} {} {}", i, text.size(), iv, dv);
      break;
    }
  }
}
} // namespace

extern "C" int LLVMFuzzerInitialize(int* /*argc*/, char*** /*argv*/)
{
  if (!g_initialized)
  {
    quill::BackendOptions backend_options;
    backend_options.transit_event_buffer_initial_capacity = 2;
    quill::Backend::start(backend_options);

    auto sink = QueueFrontend::create_or_get_sink<quill::FileSink>(
      "queue_configuration_fuzz.log",
      []()
      {
        quill::FileSinkConfig cfg;
        cfg.set_open_mode('w');
        cfg.set_filename_append_option(quill::FilenameAppendOption::None);
        return cfg;
      }(),
      quill::FileEventNotifier{});

    g_logger = QueueFrontend::create_or_get_logger("queue_configuration_fuzzer", std::move(sink));
    g_logger->set_log_level(quill::LogLevel::TraceL3);
    g_logger->set_immediate_flush(128);

    g_initialized = true;
  }

  return 0;
}

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size)
{
  if (size < 4 || !g_logger)
  {
    return 0;
  }

  FuzzDataExtractor extractor{data, size};

  switch (extractor.get_byte() % 5u)
  {
  case 0:
    log_burst(extractor, extractor.get_byte() % 48u + 1u);
    break;
  case 1:
    log_burst(extractor, extractor.get_byte() % 96u + 24u);
    break;
  case 2:
    while (extractor.remaining() > 4u)
    {
      size_t message_size = (extractor.get_byte() % 2u) == 0u ? (extractor.get_byte() % 16u + 1u)
                                                              : (extractor.get_byte() % 2048u + 256u);
      std::string payload = extractor.get_bytes(message_size);
      LOG_INFO(g_logger, "queue_sizes {} {}", message_size, payload);
    }
    break;
  case 3:
    log_burst(extractor, extractor.get_byte() % 64u + 1u);
    QueueFrontend::shrink_thread_local_queue(256u);
    break;
  case 4:
    log_burst(extractor, extractor.get_byte() % 64u + 1u);
    if (extractor.get_bool())
    {
      g_logger->flush_log();
    }
    quill::Backend::notify();
    break;
  }

  return 0;
}
