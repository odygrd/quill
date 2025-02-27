#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/CsvWriter.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

using namespace quill;

/**
 * Redirect the signal handler error to specific Logger
 */
TEST_CASE("signal_handler_logger")
{
  static constexpr char const* filename_a = "signal_handler_logger_a.log";
  static constexpr char const* filename_b = "signal_handler_logger_b.log";

  // Loggers are sorted in alphabetical order and signal handler by default can pick the first
  // but we specify a different once
  static std::string const logger_name_a = "a_logger";
  static std::string const logger_name_b = "b_logger";
  static constexpr size_t number_of_messages = 10;

  // Start the logging backend thread, we expect the signal handler to catch the signal,
  // flush the log and raise the signal back
  Backend::start<FrontendOptions>(
    BackendOptions{}, SignalHandlerOptions{std::vector<int>{SIGABRT}, 40, logger_name_b});

  // For testing purposes we want to keep the application running, we do not reraise the signal
  detail::SignalHandlerContext::instance().should_reraise_signal.store(false);

#if defined(_WIN32)
  // NOTE: On windows the signal handler must be installed on each new thread
  quill::init_signal_handler<quill::FrontendOptions>();
#endif

  Logger* logger_a = Frontend::create_or_get_logger(logger_name_a,
                                                    Frontend::create_or_get_sink<FileSink>(
                                                      filename_a,
                                                      []()
                                                      {
                                                        FileSinkConfig cfg;
                                                        cfg.set_open_mode('w');
                                                        cfg.set_fsync_enabled(true);
                                                        return cfg;
                                                      }(),
                                                      FileEventNotifier{}));

  // Create the logger for the signal handler to find it
  Frontend::create_or_get_logger(logger_name_b,
                                 Frontend::create_or_get_sink<FileSink>(
                                   filename_b,
                                   []()
                                   {
                                     FileSinkConfig cfg;
                                     cfg.set_open_mode('w');
                                     cfg.set_fsync_enabled(true);
                                     return cfg;
                                   }(),
                                   FileEventNotifier{}));

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    LOG_INFO(logger_a, "A log info message {}", i);
  }

  // Now raise a signal
  std::raise(SIGABRT);

  {
    std::vector<std::string> const file_contents_a = quill::testing::file_contents(filename_a);
    REQUIRE_EQ(file_contents_a.size(), number_of_messages);

    // Except the log and the signal handler in the logger_b
    std::vector<std::string> const file_contents_b = quill::testing::file_contents(filename_b);

#if defined(_WIN32)
    REQUIRE(quill::testing::file_contains(file_contents_b, std::string{"Received signal: 22 (signum: 22)"}));
#elif defined(__apple_build_version__)
    REQUIRE(quill::testing::file_contains(
      file_contents_b, std::string{"Received signal: Abort trap: 6 (signum: 6)"}));
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
    REQUIRE(quill::testing::file_contains(file_contents_b, std::string{"Received signal: Abort trap (signum: 6)"}));
#else
    REQUIRE(quill::testing::file_contains(file_contents_b, std::string{"Received signal: Aborted (signum: 6)"}));
#endif
  }

  // Wait until the backend thread stops for test stability
  for (Logger* logger : Frontend::get_all_loggers())
  {
    logger->flush_log();
    Frontend::remove_logger(logger);
  }

  Backend::stop();
  REQUIRE_FALSE(Backend::is_running());

  testing::remove_file(filename_a);
  testing::remove_file(filename_b);
}