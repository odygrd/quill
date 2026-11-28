#ifndef QUILL_DISABLE_NON_PREFIXED_MACROS
  #define QUILL_DISABLE_NON_PREFIXED_MACROS
#endif

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/SyslogSink.h"

#include <string>
#include <utility>

int main()
{
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Frontend
  auto sink = quill::Frontend::create_or_get_sink<quill::SyslogSink>("app",
                                                                     []()
                                                                     {
                                                                       quill::SyslogSinkConfig config;
                                                                       config.set_identifier("app");
                                                                       return config;
                                                                     }());

  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(sink));

  QUILL_LOG_INFO(logger, "A {} message with number {}", "log", 1);
  QUILL_LOG_WARNING(logger, "test message {}", 123);
}
