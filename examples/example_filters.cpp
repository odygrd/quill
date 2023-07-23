#include "quill/Quill.h"
#include "quill/filters/FilterBase.h"

/**
 * Logging to stdout and to a file using filters
 * All log levels of LOG_WARNING and above will be logged to the console handler
 */

/**
 * Filter class for our file handler
 */
class FileFilter : public quill::FilterBase
{
public:
  FileFilter() : quill::FilterBase("FileFilter"){};

  QUILL_NODISCARD bool filter(char const* thread_id, std::chrono::nanoseconds log_message_timestamp,
                              quill::MacroMetadata const& metadata,
                              quill::fmt_buffer_t const& formatted_record) noexcept override
  {
    // log only messages that are INFO or above in the file
    return metadata.level() < quill::LogLevel::Warning;
  }
};

/**
 * Filter for the stdout handler
 */
class StdoutFilter : public quill::FilterBase
{
public:
  StdoutFilter() : quill::FilterBase("StdoutFilter"){};

  QUILL_NODISCARD bool filter(char const* thread_id, std::chrono::nanoseconds log_message_timestamp,
                              quill::MacroMetadata const& metadata,
                              quill::fmt_buffer_t const& formatted_record) noexcept override
  {
    // log only WARNINGS or higher to stdout
    return metadata.level() >= quill::LogLevel::Warning;
  }
};

int main()
{
  // Start the logging backend thread
  quill::start();

  // Get a handler to the file
  // The first time this function is called a file handler is created for this filename.
  // Calling the function with the same filename will return the existing handler
  std::shared_ptr<quill::Handler> file_handler = quill::file_handler("example_filters.log",
                                                                     []()
                                                                     {
                                                                       quill::FileHandlerConfig cfg;
                                                                       cfg.set_open_mode('w');
                                                                       return cfg;
                                                                     }());

  // Create and add the filter to our handler
  file_handler->add_filter(std::make_unique<FileFilter>());

  // Also create an stdout handler
  std::shared_ptr<quill::Handler> stdout_handler = quill::stdout_handler("stdout_1");

  // Create and add the filter to our handler
  stdout_handler->add_filter(std::make_unique<StdoutFilter>());

  // Create a logger using this handler
  quill::Logger* logger = quill::create_logger("logger", {file_handler, stdout_handler});

  // Change the LogLevel to print everything
  logger->set_log_level(quill::LogLevel::TraceL3);

  // Log to both handlers
  LOG_TRACE_L3(logger, "This is a log trace l3 example {}", 1);
  LOG_TRACE_L2(logger, "This is a log trace l2 example {} {}", 2, 2.3);
  LOG_TRACE_L1(logger, "This is a log trace l1 example {}", 3);
  LOG_DEBUG(logger, "This is a log debug example {}", 4);
  LOG_INFO(logger, "This is a log info example {}", 5);
  LOG_WARNING(logger, "This is a log warning example {}", 6);
  LOG_ERROR(logger, "This is a log error example {}", 7);
  LOG_CRITICAL(logger, "This is a log critical example {}", 8);
}
