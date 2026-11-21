#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <utility>

/** The singletons of this DLL, in the order DllSharedSingletonsTest compares them */
QUILL_EXPORT void singletons_dll_instances(void const** instances)
{
  instances[0] = &quill::detail::LoggerManager::instance();
  instances[1] = &quill::detail::SinkManager::instance();
  instances[2] = &quill::detail::ThreadContextManager::instance();
  instances[3] = &quill::detail::MetricManager::instance();
  instances[4] = &quill::detail::BackendManager::instance();
  instances[5] = &quill::detail::SignalHandlerContext::instance();
  instances[6] = &quill::detail::RdtscClock::RdtscTicks::instance();
}

QUILL_EXPORT void singletons_dll_start_backend() { quill::Backend::start(); }

/** Gets the sink of `filename` through its name and creates a logger that writes to it */
QUILL_EXPORT quill::Logger* singletons_dll_create_logger(char const* logger_name,
                                                         char const* filename, quill::Sink** sink)
{
  auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(filename);
  *sink = file_sink.get();
  return quill::Frontend::create_or_get_logger(logger_name, std::move(file_sink));
}

QUILL_EXPORT void singletons_dll_log(quill::Logger* logger, char const* message)
{
  LOG_INFO(logger, "{}", message);
}
