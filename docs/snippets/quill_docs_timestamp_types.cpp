#include "quill/Backend.h"
#include "quill/BackendTscClock.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

#include <iostream>
#include <utility>

int main()
{
  quill::Backend::start();

  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");

  // Ensure at least one logger with quill::ClockSourceType::Tsc is created for BackendTscClock to function
  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "root", std::move(console_sink),
    quill::PatternFormatterOptions{
      "%(time) [%(thread_id)] %(short_source_location:<28) LOG_%(log_level:<9) %(logger:<12) "
      "%(message)",
      "%H:%M:%S.%Qns", quill::Timezone::LocalTime},
    quill::ClockSourceType::Tsc);

  // Log an informational message which will also init the backend RdtscClock
  LOG_INFO(logger, "This is a log info example with number {}", 123);

  // The function `quill::detail::BackendManager::instance().convert_rdtsc_to_epoch_time(quill::detail::rdtsc())`
  // will return a valid timestamp only after the backend worker has started and processed
  // at least one log with `ClockSourceType::Tsc`.
  // This is because the Rdtsc clock is lazily initialized by the backend worker on the first log message.
  // To ensure at least one log message is processed, we call flush_log here.
  logger->flush_log();

  // Get a timestamp synchronized with the backend's clock
  uint64_t const backend_timestamp = quill::BackendTscClock::now().time_since_epoch().count();
  std::cout << "Synchronized timestamp with the backend: " << backend_timestamp << std::endl;

  return 0;
}