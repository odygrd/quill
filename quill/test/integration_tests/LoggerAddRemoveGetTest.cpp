#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace quill;

/**
 * A test class example that is using the logger
 */
class LoggingTestClass
{
public:
  LoggingTestClass(std::string const& filename, std::string const& logger_name)
  {
    // create a new logger in the ctor
    auto file_sink = Frontend::create_or_get_sink<FileSink>(
      filename,
      []()
      {
        FileSinkConfig cfg;
        cfg.set_open_mode('w');
        cfg.set_do_fsync(true);
        return cfg;
      }(),
      FileEventNotifier{});

    quill::Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));
    _logger = logger;
  }

  ~LoggingTestClass()
  {
    _logger->flush_log();
    Frontend::remove_logger(_logger);
  }

  /**
   * Use logger in const function
   */
  void use_logger_const() const noexcept { LOG_INFO(_logger, "Test message for test class const"); }

  /**
   * Use logger in normal function
   */
  void use_logger() const { LOG_INFO(_logger, "Test message for test class non const"); }

private:
  quill::Logger* _logger{nullptr};
};

/***/
TEST_CASE("logger_add_remove_get")
{
  // Verifies logging behavior adding, removing and getting loggers
  // and logging from const member functions while dynamically creating and
  // removing loggers and reusing the same file after removal.
  static constexpr size_t iterations = 100;
  static constexpr char const* filename = "logger_add_remove_get.log";

  // Start the logging backend thread
  Backend::start();

  for (size_t i = 0; i < iterations; ++i)
  {
    std::string const logger_a_name = "logger_a" + std::to_string(i);
    std::string const logger_b_name = "logger_b" + std::to_string(i);

    {
      // log for class a
      LoggingTestClass logging_test_class_a{filename, logger_a_name};
      logging_test_class_a.use_logger_const();
      logging_test_class_a.use_logger();

      // log again for class b
      LoggingTestClass const logging_test_class_b{filename, logger_b_name};
      logging_test_class_b.use_logger_const();
    }

    // Test that get_all_loggers() works and also it should be empty after removing the loggers
    REQUIRE(Frontend::get_all_loggers().empty());
    REQUIRE(!Frontend::get_logger(logger_a_name));
    REQUIRE(!Frontend::get_logger(logger_b_name));

    // The only safe way to know that the loggers are really removed is currently this
    while (Frontend::get_number_of_loggers())
    {
      // wait for all the to be removed first by the backend thread first then repeat the test
      std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }

    // Read file and check
    std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
    REQUIRE_EQ(file_contents.size(), 3);

    testing::remove_file(filename);
  }

  Backend::stop();
}