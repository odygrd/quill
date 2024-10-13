#ifdef NDEBUG
#else
  #define QUILL_IMMEDIATE_FLUSH true
  #define QUILL_COMPILE_ACTIVE_LOG_LEVEL QUILL_COMPILE_ACTIVE_LOG_LEVEL_TRACE_L3
#endif

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"
#include "quill/sinks/FileSink.h"

#include <iostream>
#include <string>
#include <utility>

/**
 * Trivial logging example to console
 * Note: You can also pass STL types by including the relevant header files from quill/std/
 */

int main()
{
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // log everything to console
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>(
    "console_sink");
  // log everything to file
  auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
    "debug_logs.txt", []() {
      quill::FileSinkConfig cfg;
      cfg.set_open_mode('w');
      cfg.set_filename_append_option(
        quill::FilenameAppendOption::StartDateTime);
      return cfg;
    }(),
    quill::FileEventNotifier{});

  // create logger
  quill::Logger *logger = quill::Frontend::create_or_get_logger(
    "root", {std::move(console_sink), std::move(file_sink)});

  // Change the LogLevel to print everything
  logger->set_log_level(quill::LogLevel::TraceL3);

  // A log message with number 123
  int a = 123;
  std::string l = "log";
  LOG_INFO(logger, "A {} message with number {}", l, a);

  // libfmt formatting language is supported 3.14e+00
  double pi = 3.141592653589793;
  LOG_INFO(logger, "libfmt formatting language is supported {:.2e}", pi);

  // A message with two variables [a: 123, b: 3.17]
  double b = 3.17;
  LOGV_NOTICE(logger, "A message with two variables", a, b);

  for (uint32_t i = 0; i < 10; ++i)
  {
    // Will only log the message once per second
    LOG_INFO_LIMIT(std::chrono::seconds{1}, logger, "A {} message with number {}", l, a);
    LOGV_INFO_LIMIT(std::chrono::seconds{1}, logger, "A message with two variables", a, b);
  }
}
