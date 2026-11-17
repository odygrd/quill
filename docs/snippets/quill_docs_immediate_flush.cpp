#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/ConsoleSink.h"

struct AppFrontendOptions : quill::FrontendOptions
{
  static constexpr bool enable_immediate_flush = false;
};

using AppFrontend = quill::FrontendImpl<AppFrontendOptions>;
using AppLogger = AppFrontend::logger_t;

int main()
{
  quill::Backend::start();
  auto sink = AppFrontend::create_or_get_sink<quill::ConsoleSink>("no_immediate_flush_console");
  AppLogger* logger = AppFrontend::create_or_get_logger("no_immediate_flush_logger", sink);

  LOG_INFO(logger, "Immediate-flush checks are compiled out");

  // Explicit flushing is available even when immediate flushing is disabled.
  logger->flush_log();
  AppFrontend::remove_logger_blocking(logger);
  quill::Backend::stop();
}
