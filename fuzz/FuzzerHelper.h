#pragma once

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/Logger.h"
#include "quill/sinks/FileSink.h"

#include <string>

#ifndef FUZZER_LOG_FILENAME
#error "FUZZER_LOG_FILENAME must be defined before including FuzzerHelper.h"
#endif

struct CustomFrontendOptions
{
  static constexpr quill::QueueType queue_type = quill::QueueType::UnboundedBlocking;
  static constexpr size_t initial_queue_capacity = 4096; // start low for extra testing
  static constexpr uint32_t blocking_queue_retry_interval_ns = 800;
  static constexpr size_t unbounded_queue_max_capacity = 2ull * 1024u * 1024u * 1024u;
  static constexpr quill::HugePagesPolicy huge_pages_policy = quill::HugePagesPolicy::Never;
};

// Global logger instance
static quill::Logger* g_logger = nullptr;
static bool g_initialized = false;

extern "C" int LLVMFuzzerInitialize(int* /*argc*/, char*** /*argv*/)
{
  if (!g_initialized)
  {
    // Low the limits for additional testing
    quill::BackendOptions backend_options;
    backend_options.transit_event_buffer_initial_capacity = 2;

#ifdef FUZZER_USE_BINARY_MODE
    // Binary mode: disable printable char check
    backend_options.check_printable_char = {};
#endif

    quill::Backend::start(backend_options);

#ifdef FUZZER_USE_BINARY_MODE
    // Binary pattern formatter - no suffix for raw binary data
    quill::PatternFormatterOptions pattern_options;
    pattern_options.format_pattern = "%(message)";
    pattern_options.add_metadata_to_multi_line_logs = false;
    pattern_options.pattern_suffix = quill::PatternFormatterOptions::NO_SUFFIX;
#endif

    auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
      FUZZER_LOG_FILENAME,
      []()
      {
        quill::FileSinkConfig cfg;
#ifdef FUZZER_USE_BINARY_MODE
        cfg.set_open_mode("wb"); // Binary mode
#else
        cfg.set_open_mode('w'); // Text mode
#endif
        cfg.set_filename_append_option(quill::FilenameAppendOption::None);
        return cfg;
      }(),
      quill::FileEventNotifier{});

#ifdef FUZZER_USE_BINARY_MODE
    g_logger = quill::Frontend::create_or_get_logger("root", std::move(file_sink), pattern_options);
#else
    g_logger = quill::Frontend::create_or_get_logger("root", std::move(file_sink));
#endif

    g_logger->set_log_level(quill::LogLevel::TraceL3);

    // Allow customization of flush limit via preprocessor
    // Higher limits allow more queue stress but take longer to run
#ifndef FUZZER_IMMEDIATE_FLUSH_LIMIT
    g_logger->set_immediate_flush(2048); // default: we need a limit otherwise the fuzzer produces too much input
#else
    g_logger->set_immediate_flush(FUZZER_IMMEDIATE_FLUSH_LIMIT);
#endif

    g_initialized = true;
  }
  return 0;
}

