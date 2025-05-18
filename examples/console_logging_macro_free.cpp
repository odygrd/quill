#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogFunctions.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

#include <string>
#include <utility>

/**
 * Trivial logging example to console
 * Note: You can also pass STL types by including the relevant header files from quill/std/
 */

int main()
{
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Frontend
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");

  // [ %(tags)] is for demonstration purposes only, you might remove it if you don't want it
  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "root", std::move(console_sink),
    quill::PatternFormatterOptions{"%(time) [%(thread_id)] %(short_source_location:<28) "
                                   "LOG_%(log_level:<9) %(logger:<12) [ %(tags)] %(message)"});

  // Change the LogLevel to print everything
  logger->set_log_level(quill::LogLevel::TraceL3);

  int a = 123;
  std::string l = "log";

  quill::tracel3(logger, "A {} message with number {}", l, a);
  quill::tracel3(logger, quill::Tags{"TAG"}, "A {} message with number {}", l, a);

  quill::tracel2(logger, "A {} message with number {}", l, a);
  quill::tracel2(logger, quill::Tags{"TAG"}, "A {} message with number {}", l, a);

  quill::tracel1(logger, "A {} message with number {}", l, a);
  quill::tracel1(logger, quill::Tags{"TAG"}, "A {} message with number {}", l, a);

  quill::debug(logger, "A {} message with number {}", l, a);
  quill::debug(logger, quill::Tags{"TAG"}, "A {} message with number {}", l, a);

  quill::info(logger, "A {} message with number {}", l, a);
  quill::info(logger, quill::Tags{"TAG"}, "A {} message with number {}", l, a);

  quill::notice(logger, "A {} message with number {}", l, a);
  quill::notice(logger, quill::Tags{"TAG"}, "A {} message with number {}", l, a);

  quill::warning(logger, "A {} message with number {}", l, a);
  quill::warning(logger, quill::Tags{"TAG"}, "A {} message with number {}", l, a);

  quill::error(logger, "A {} message with number {}", l, a);
  quill::error(logger, quill::Tags{"TAG"}, "A {} message with number {}", l, a);

  quill::critical(logger, "A {} message with number {}", l, a);
  quill::critical(logger, quill::Tags{"TAG"}, "A {} message with number {}", l, a);
}
