#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

using namespace quill;

/***/
TEST_CASE("signal_handler")
{
  static constexpr char const* filename = "signal_handler.log";
  static std::string const logger_name = "logger";
  static constexpr size_t number_of_messages = 10;
  static constexpr size_t number_of_threads = 10;
  static std::string const logger_name_prefix = "logger_";

  // Start the logging backend thread, we expect the signal handler to catch the signal,
  // flush the log and raise the signal back
  Backend::start_with_signal_handler<FrontendOptions>(BackendOptions{},
                                                      std::initializer_list<int>{SIGABRT}, 40);

  // For testing purposes we want to keep the application running, we do not reraise the signal
  detail::SignalHandlerContext::instance().should_reraise_signal.store(false);

  quill::Frontend::preallocate();

  std::vector<std::thread> threads;

  for (size_t i = 0; i < number_of_threads; ++i)
  {
    threads.emplace_back(
      [i]() mutable
      {
#if defined(_WIN32)
        // NOTE: On windows the signal handler must be installed on each new thread
        quill::init_signal_handler<quill::FrontendOptions>();
#endif

        // Set writing logging to a file
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

        Logger* logger =
          Frontend::create_or_get_logger(logger_name_prefix + std::to_string(i), std::move(file_sink));

        for (size_t j = 0; j < number_of_messages; ++j)
        {
          LOG_INFO(logger, "{} {} {}", i, j,
                   "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor "
                   "incididunt ut labore et dolore magna aliqua.");
        }
      });
  }

  for (auto& elem : threads)
  {
    elem.join();
  }

  // Now raise a signal
  std::raise(SIGABRT);

  {
    // Except the log and the signal handler in the logs
    std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

    for (size_t i = 0; i < number_of_threads; ++i)
    {
      // for each thread
      for (size_t j = 0; j < number_of_messages; ++j)
      {
        std::string expected_string = logger_name_prefix + std::to_string(i) + "     " +
          std::to_string(i) + " " + std::to_string(j) +
          " Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor "
          "incididunt ut labore et dolore magna aliqua.";

        REQUIRE(testing::file_contains(file_contents, expected_string));
      }
    }

#if defined(_WIN32)
    REQUIRE(quill::testing::file_contains(file_contents, std::string{"Received signal: 22 (signum: 22)"}));
#elif defined(__apple_build_version__)
    REQUIRE(quill::testing::file_contains(file_contents, std::string{"Received signal: Abort trap: 6 (signum: 6)"}));
#elif defined(__FreeBSD__)
    REQUIRE(quill::testing::file_contains(file_contents, std::string{"Received signal: Abort trap (signum: 6)"}));
#else
    REQUIRE(quill::testing::file_contains(file_contents, std::string{"Received signal: Aborted (signum: 6)"}));
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

  testing::remove_file(filename);
}