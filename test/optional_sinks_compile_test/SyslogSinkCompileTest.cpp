// syslog.h defines LOG_INFO and friends, so use the prefixed Quill macros
#define QUILL_DISABLE_NON_PREFIXED_MACROS

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/SyslogSink.h"

#include <utility>

int main()
{
  quill::Backend::start();

  quill::SyslogSinkConfig sink_config;
  sink_config.set_identifier("quill_syslog_sink_compile_test");

  auto syslog_sink =
    quill::Frontend::create_or_get_sink<quill::SyslogSink>("syslog_sink", std::move(sink_config));

  quill::Logger* logger = quill::Frontend::create_or_get_logger("syslog_logger", std::move(syslog_sink));

  QUILL_LOG_INFO(logger, "Syslog compile test {}", 1);

  logger->flush_log();
  quill::Frontend::remove_logger(logger);
  quill::Backend::stop();
}
