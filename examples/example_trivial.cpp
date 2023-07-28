#include "quill/Quill.h"

#include <chrono>
#include <thread>

/**
 * Trivial logging example
 */

/**
 * If you do not wish to pass Logger* objects around you can just create your own macros likes this
 */
#define MY_LOG_INFO(fmt, ...) QUILL_LOG_INFO(quill::get_root_logger(), fmt, ##__VA_ARGS__)

int main()
{
  // Start the logging backend thread
  quill::start();

  MY_LOG_INFO("Hello quill!");

  // a) We can use the default logger like this
  {
    // Using the default logger.
    // Default handler : stdout
    // Default LogLevel : Info
    // Default pattern : "%(ascii_time) [%(thread)] %(filename):%(lineno) %(level_name) %(logger_name) - %(message)"
    quill::Logger* logger = quill::get_logger();

    // Change the LogLevel to print everything
    logger->set_log_level(quill::LogLevel::TraceL3);

    LOG_TRACE_L3(logger, "This is a log trace l3 example {}", 1);
    LOG_TRACE_L2(logger, "This is a log trace l2 example {} {}", 2, 2.3);
    LOG_TRACE_L1(logger, "This is a log trace l1 {} example", "string");
    LOG_DEBUG(logger, "This is a log debug example {}", 4);
    LOG_INFO(logger, "This is a log info example {}", 5);
    LOG_WARNING(logger, "This is a log warning example {}", 6);
    LOG_ERROR(logger, "This is a log error example {}", 7);
    LOG_CRITICAL(logger, "This is a log critical example {}", 8);

    std::array<uint32_t, 4> arr = {1, 2, 3, 4};
    LOG_INFO(logger, "This is a log info example {}", arr);

    // Using a dynamic runtime log level
    std::array<quill::LogLevel, 4> const runtime_log_levels = {
      quill::LogLevel::Debug, quill::LogLevel::Info, quill::LogLevel::Warning, quill::LogLevel::Error};

    for (auto const& log_level : runtime_log_levels)
    {
      LOG_DYNAMIC(logger, log_level, "Runtime {} {}", "log", "level");
    }

    // printf format style is also supported
    LOG_INFO_CFORMAT(logger, "printf style %s %d %f", "example", 5, 2.31);
  }

  // b) Or like this
  {
    LOG_TRACE_L3(quill::get_logger(), "This is a log trace l3 example {}", 1);
    LOG_TRACE_L2(quill::get_logger(), "This is a log trace l2 example {} {}", 2, 2.3);
    LOG_TRACE_L1(quill::get_logger(), "This is a log trace l1 example {}", 3);
    LOG_DEBUG(quill::get_logger(), "This is a log debug example {}", 4);
    LOG_INFO(quill::get_logger(), "This is a log info example {}", 5);
    LOG_WARNING(quill::get_logger(), "This is a log warning example {}", 6);
    LOG_ERROR(quill::get_logger(), "This is a log error example {}", 7);
    LOG_CRITICAL(quill::get_logger(), "This is a log critical example {}", 8);
  }

  // c) Or like this
  {
#define DEF_LOG_INFO(fmt, ...) LOG_INFO(quill::get_logger(), fmt, ##__VA_ARGS__)

    quill::get_logger()->set_log_level(quill::LogLevel::Info);
    DEF_LOG_INFO("This is a log info example {}", 5);
  }

  // d) Basic Usage
  // Get a pointer to the default logger
  quill::Logger* default_logger = quill::get_logger();

  LOG_INFO(default_logger, "Welcome to Quill!");
  LOG_INFO(default_logger, "Print a vector {} ", std::vector<int>{1, 2, 3, 4, 5});

#if QUILL_FMT_VERSION >= 100000
  LOG_INFO(default_logger, "or some optionals [{}, {}]", std::optional<std::string>{},
           std::optional<std::string>{"hello"});
#endif

  LOG_ERROR(default_logger, "An error message with error code {}, error message {}", 123,
            "system_error");

  LOG_WARNING(default_logger, "Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);
  LOG_CRITICAL(default_logger, "Easy padding in numbers like {:08d}", 12);

  LOG_DEBUG(default_logger,
            "This message and any message below this log level will not be displayed..");

  // Enable additional log levels
  default_logger->set_log_level(quill::LogLevel::TraceL3);

  LOG_DEBUG(default_logger, "The answer is {}", 1337);
  LOG_TRACE_L1(default_logger, "{:>30}", "right aligned");
  LOG_TRACE_L2(default_logger, "Positional arguments are {1} {0} ", "too", "supported");
  LOG_TRACE_L3(default_logger, "Support for floats {:03.2f}", 1.23456);

  // Log Nothing
  quill::Logger* logger_1 = quill::create_logger("my_logger");
  logger_1->set_log_level(quill::LogLevel::None);
  LOG_CRITICAL(logger_1, "This is never logged");

  // Get all created loggers
  std::unordered_map<std::string, quill::Logger*> created_loggers = quill::get_all_loggers();
  std::vector<std::string> logger_names;
  for (auto const& elem : created_loggers)
  {
    logger_names.emplace_back(elem.first);
  }

  LOG_INFO(default_logger, "Existing logger names {}", logger_names);

  for (uint64_t i = 0; i < 10; ++i)
  {
    LOG_INFO_LIMIT(std::chrono::milliseconds{100}, default_logger,
                   "log in a loop with limit 1 message every 100 ms for i {}", i);
    LOG_DEBUG_LIMIT(std::chrono::seconds{1}, default_logger,
                    "log in a loop with limit 1 message every 1 second for i {}", i);
    std::this_thread::sleep_for(std::chrono::microseconds{30});
  }
}
