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

#if !defined(QUILL_NO_EXCEPTIONS)

/***/
TEST_CASE("backend_exception_notifier")
{
  static constexpr char const* filename = "backend_exception_notifier.log";

  // Set writing logging to a file
  auto file_sink = Frontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger("logger", std::move(file_sink));

  // counter to check our error handler was invoked
  // atomic because we check this value on this thread, but the backend worker thread updates it
  std::atomic<size_t> error_notifier_invoked{0};

  // Set invalid thread name
  BackendOptions backend_options;

  #if !defined(__FreeBSD__)
  // On FreeBSD CPU_SET(cpu_id, &cpuset); with a big number crashes.
  // Setting to an invalid CPU. When we call quill::start() our error handler will be invoked and an error will be logged
  backend_options.cpu_affinity = static_cast<uint16_t>(std::numeric_limits<uint16_t>::max() - 1);
  #endif

  backend_options.thread_name =
    "Lorem_ipsum_dolor_sit_amet_consectetur_adipiscing_elit_sed_do_eiusmod_tempor_incididunt_ut_"
    "labore_et_dolore_magna_aliqua";

  backend_options.error_notifier = [logger, &error_notifier_invoked](std::string const& error_message)
  {
    // Log inside the function from the backend thread, for testing

    // Note that this log is asynchronous here which means when error_notifier_invoked is
    // incremented does not mean we have logged this yet

    // flush_log() is not permitted inside this callback
    LOG_WARNING(logger, "error handler invoked {}", error_message);

    error_notifier_invoked.fetch_add(1);
  };

  Backend::start(backend_options);

  // Log a message and wait for it to get processed, that way we know the backend thread has started
  LOG_INFO(logger, "frontend");
  logger->flush_log();

  #if !defined(__FreeBSD__)
  // Check our handler was invoked since either set_backend_thread_name or set_backend_thread_cpu_affinity should have failed
  REQUIRE_GE(error_notifier_invoked.load(), 1);
  #endif

  // Now we can try to get another exception by calling LOG_BACKTRACE without calling init first
  error_notifier_invoked.store(0);

  LOG_BACKTRACE(logger, "Backtrace message");
  logger->flush_log();

  // Check our handler was invoked
  REQUIRE_EQ(error_notifier_invoked.load(), 1);

  // Pass an invalid fmt format and see if it throws
  error_notifier_invoked.store(0);

  LOG_INFO(logger, "Format {:>321.}", 321.3);
  logger->flush_log();

  // Check our handler was invoked
  REQUIRE_EQ(error_notifier_invoked.load(), 1);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // After the backend has stopped, all messages include the async ones from the notifier will
  // be in the log file. At this point we can safely check it

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  #if !defined(__FreeBSD__)
  // Look for the async errors
  std::string const expected_string_1 = "error handler invoked Failed to set cpu affinity ";
  std::string const expected_string_2 = "error handler invoked Failed to set thread name ";

  bool const has_any_error = quill::testing::file_contains(file_contents, expected_string_1) ||
    quill::testing::file_contains(file_contents, expected_string_2);

  REQUIRE(has_any_error);
  #endif
  
  std::string const expected_string_3 = "error handler invoked logger->init_backtrace(...)";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string_3));

  std::string const expected_string_4 = "error handler invoked [Could not format log statement.";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string_4));

  testing::remove_file(filename);
}

#endif