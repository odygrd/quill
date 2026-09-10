// GCC/libstdc++ needs these declarations before the module import.
#include <new>
#include <typeinfo>
#include <utility>

import quill;

#define QUILL_USE_MODULE
#include "quill/LogMacros.h"

int main()
{
  quill::Backend::start();

  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("module_docs_sink");
  quill::Logger* logger =
    quill::Frontend::create_or_get_logger("module_docs_logger", std::move(console_sink));

  LOG_INFO(logger, "Logging through the Quill module: {}", 42);
  logger->flush_log();
  quill::Backend::stop();
}
