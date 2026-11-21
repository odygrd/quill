// sd-journal.h pulls in syslog.h, which defines LOG_INFO and friends
#define QUILL_DISABLE_NON_PREFIXED_MACROS

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/SystemdSink.h"

#include <utility>

int main()
{
  quill::Backend::start();

  quill::SystemdSinkConfig sink_config;
  sink_config.set_identifier("quill_systemd_sink_compile_test");

  auto systemd_sink =
    quill::Frontend::create_or_get_sink<quill::SystemdSink>("systemd_sink", std::move(sink_config));

  quill::Logger* logger = quill::Frontend::create_or_get_logger("systemd_logger", std::move(systemd_sink));

  QUILL_LOG_INFO(logger, "Systemd compile test {}", 1);

  logger->flush_log();
  quill::Frontend::remove_logger(logger);
  quill::Backend::stop();
}
