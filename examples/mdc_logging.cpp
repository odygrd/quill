#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

#include <string>
#include <utility>

/**
 * This example shows how to attach Mapped Diagnostic Context (MDC) to a frontend thread so that
 * subsequent log statements from the same thread automatically carry request-scoped or
 * task-scoped context such as request ids, user ids, or tenant ids.
 *
 * MDC updates are queued as a single control event per change; later LOG_* calls do not
 * re-serialize or re-enqueue MDC fields on the hot path. The backend remembers the current MDC
 * state per frontend thread and reuses it for subsequent log lines.
 *
 * The default pattern formatter already includes `%(mdc)` at the end of `%(message)`. When no
 * MDC is set, `%(mdc)` expands to an empty string.
 */

int main()
{
  quill::Backend::start();

  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");

  // The default pattern already contains `%(mdc)`. A custom pattern only needs to place
  // `%(mdc)` immediately after `%(message)` (no space between them), so the MDC block's
  // leading space is only printed when fields are present.
  quill::Logger* app_logger = quill::Frontend::create_or_get_logger(
    "app", std::move(console_sink),
    quill::PatternFormatterOptions{
      "%(time) [%(thread_id)] %(logger:<6) %(log_level:<9) %(message)%(mdc)"});

  // Multiple loggers on the same thread share the same MDC — MDC is per frontend thread, not
  // per logger instance.
  quill::Logger* db_logger = quill::Frontend::create_or_get_logger("db", app_logger);

  // Attach request-scoped context once. Every subsequent log statement from this thread picks
  // it up automatically until it is replaced, erased, or cleared.
  app_logger->set_mdc("request_id", 42, "user", "alice");

  LOG_INFO(app_logger, "request started");
  LOG_INFO(db_logger, "querying database");

  // Update a single field. MDC stores its own copy of the value, so updating the local
  // variable alone would not change the rendered MDC.
  app_logger->set_mdc("user", "bob");
  LOG_INFO(app_logger, "user switched");

  // Remove a single field.
  app_logger->erase_mdc("user");
  LOG_INFO(app_logger, "user removed from context");

  // Clear everything for the current thread. Other threads are unaffected.
  app_logger->clear_mdc();
  LOG_INFO(db_logger, "request finished");
}
