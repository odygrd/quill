#include "Foo.h"

#include "quill_wrapper/QuillWrapper.h"

#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"

#include <thread>

extern quill::Logger* global_logger_a;

void init() { setup_quill("quill_shared_lib_test.log"); }

void log()
{
  // Get the logger and log
  quill::Logger* logger = quill::Frontend::get_logger("root");
  LOG_INFO(logger, "log with logger from {}", "foo");

  // Log via the global logger
  LOG_INFO(global_logger_a, "log with global logger from {}", "foo");

  std::thread t1{[logger]() { LOG_INFO(logger, "log with logger from {}", "foo thread"); }};
  t1.join();
}

void stop() { stop_quill(); }
