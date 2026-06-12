// GCC/libstdc++ needs these declarations visible before importing the Quill module.
#include <new>
#include <typeinfo>
#include <utility>

import quill;

#define QUILL_USE_MODULE
#include "quill/LogMacros.h"

/**
 * Trivial logging example to console using the Quill named module.
 */

int main()
{
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

  logger->set_log_level(quill::LogLevel::Info);
  LOG_INFO(logger, "Console logging via C++20 module import");

  quill::utility::StringRef message{"StringRef logging via module import"};
  LOG_INFO(logger, "{}", message);
}
