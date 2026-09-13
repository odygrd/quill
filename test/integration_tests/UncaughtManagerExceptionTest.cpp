#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <memory>
#include <string>
#include <thread>

#if !defined(_WIN32)
  #include <sys/resource.h>
  #include <sys/wait.h>
  #include <unistd.h>
#endif

TEST_CASE("uncaught_manager_exception_allows_terminate_flush")
{
#if defined(_WIN32) || defined(QUILL_NO_EXCEPTIONS) || !defined(QUILL_ENABLE_EXTENSIVE_TESTS) ||   \
  QUILL_HAS_FEATURE(thread_sanitizer) || defined(__SANITIZE_THREAD__)
  // This fork-based exception test is opt-in and unsupported under TSan.
  return;
#else
  static constexpr char const* filename = "uncaught_manager_exception.log";
  static constexpr char const* logger_name = "uncaught_manager_exception_logger";

  for (int scenario{0}; scenario < 4; ++scenario)
  {
    CAPTURE(scenario);
    pid_t const child = fork();
    REQUIRE_NE(child, -1);

    if (child == 0)
    {
      rlimit const core_limit{0, 0};
      setrlimit(RLIMIT_CORE, &core_limit);
      std::signal(SIGALRM, SIG_DFL);
      alarm(10);

      quill::Backend::start();
      quill::FileSinkConfig config;
      config.set_open_mode('w');
      auto sink = quill::Frontend::create_sink<quill::FileSink>(filename, config);
      quill::Frontend::create_logger(logger_name, sink);

      std::set_terminate(
        []()
        {
          // Both manager lookups and the backend flush must remain usable.
          (void)quill::Frontend::get_sink(filename);
          auto* logger = quill::Frontend::get_logger(logger_name);
          LOG_CRITICAL(logger, "uncaught manager exception flushed");
          logger->flush_log();
          std::_Exit(0);
        });

      // Doctest catches exceptions on the test thread; a fresh thread leaves this one uncaught.
      std::thread throwing_thread(
        [scenario, sink]()
        {
          quill::Frontend::preallocate();
          switch (scenario)
          {
          case 0:
            quill::Frontend::create_logger(logger_name, sink);
            break;
          case 1:
            quill::Frontend::create_or_get_logger("uncaught_manager_exception_null_sink",
                                                  std::shared_ptr<quill::Sink>{});
            break;
          case 2:
            (void)quill::Frontend::get_sink("uncaught_manager_exception_missing_sink");
            break;
          case 3:
            quill::Frontend::create_sink<quill::FileSink>(filename);
            break;
          }
        });
      throwing_thread.join();
      std::_Exit(1);
    }

    int status{0};
    REQUIRE_EQ(waitpid(child, &status, 0), child);
    REQUIRE(WIFEXITED(status));
    REQUIRE_EQ(WEXITSTATUS(status), 0);

    auto const lines = quill::testing::file_contents(filename);
    REQUIRE_EQ(lines.size(), 1);
    CHECK_NE(lines.front().find("uncaught manager exception flushed"), std::string::npos);
    std::remove(filename);
  }
#endif
}
