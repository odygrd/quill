#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/ConsoleSink.h"

int main()
{
  quill::Backend::start();

  auto sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink");

  quill::Logger* logger_a = quill::Frontend::create_or_get_logger(
    "app", std::move(sink), quill::PatternFormatterOptions{"%(time) %(logger) %(message)%(mdc)"});
  quill::Logger* logger_b = quill::Frontend::create_or_get_logger("db", logger_a);

  logger_a->set_mdc("request_id", 42, "user", "alice");
  LOG_INFO(logger_a, "request started");
  LOG_INFO(logger_b, "querying database");

  logger_b->erase_mdc("user");
  LOG_INFO(logger_a, "user removed from context");

  logger_a->clear_mdc();
  LOG_INFO(logger_b, "request finished");

  logger_a->flush_log();
}
