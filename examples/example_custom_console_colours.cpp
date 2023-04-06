#include "quill/Quill.h"

/**
 * Example on how to create a new stdout handler with console colours or
 * customise the console colours.
 */

int main()
{
  // Create a ConsoleColours class that we will pass to `quill::stdout_handler(...)`
  quill::ConsoleColours console_colours;

  // Enable using default colours to get the default colour scheme
  console_colours.set_default_colours();

  // Colours can be customised
  console_colours.set_colour(quill::LogLevel::Info, quill::ConsoleColours::white);

  // Get the stdout sink.
  // stdout_handler_name should NOT be the default "stdout" when using colours
  std::shared_ptr<quill::Handler> stdout_handler = quill::stdout_handler("stdout_colours", console_colours);

  // set the handler as default for all loggers
  quill::Config cfg;
  cfg.default_handlers.emplace_back(stdout_handler);
  quill::configure(cfg);

  // Start the logging backend thread
  quill::start();

  // Get default logger
  quill::Logger* logger = quill::get_logger();

  // Can also create a logger using this handler
  // quill::Logger* logger = quill::create_logger("colour_logger", stdout_handler);

  // Change the LogLevel to print everything
  logger->set_log_level(quill::LogLevel::TraceL3);

  // We also use backtrace
  logger->init_backtrace(2, quill::LogLevel::Critical);
  LOG_BACKTRACE(logger, "Backtrace log {}", 1);
  LOG_BACKTRACE(logger, "Backtrace log {}", 2);

  LOG_TRACE_L3(logger, "This is a log trace l3 example {}", 1);
  LOG_TRACE_L2(logger, "This is a log trace l2 example {} {}", 2, 2.3);
  LOG_TRACE_L1(logger, "This is a log trace l1 example {}", 3);
  LOG_DEBUG(logger, "This is a log debug example {}", 4);
  LOG_INFO(logger, "This is a log info example {}", 5);
  LOG_WARNING(logger, "This is a log warning example {}", 6);
  LOG_ERROR(logger, "This is a log error example {}", 7);
  LOG_CRITICAL(logger, "This is a log critical example {}", 8);
}