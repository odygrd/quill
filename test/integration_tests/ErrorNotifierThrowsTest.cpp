#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/bundled/fmt/format.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

using namespace quill;

TEST_CASE("error_notifier_throws")
{
#if defined(QUILL_NO_EXCEPTIONS)
  return;
#else
  static constexpr char const* filename = "error_notifier_throws.log";

  auto file_sink = Frontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger("logger_err_notifier_throws", std::move(file_sink));

  BackendOptions backend_options;

  // error_notifier that always throws — exercises the catch-all inside _notify_error
  // for every code path that reports errors.
  backend_options.error_notifier = [](std::string const&)
  { throw std::runtime_error{"error_notifier_throws_test"}; };

  // Poll hooks that throw — these are caught and routed through _notify_error,
  // which will also throw. The backend must survive both.
  std::atomic<bool> on_poll_begin_thrown{false};
  std::atomic<bool> on_poll_end_thrown{false};

  backend_options.backend_worker_on_poll_begin = [&on_poll_begin_thrown]()
  {
    if (!on_poll_begin_thrown.exchange(true))
    {
      throw std::runtime_error{"poll_begin_throws"};
    }
  };

  backend_options.backend_worker_on_poll_end = [&on_poll_end_thrown]()
  {
    if (!on_poll_end_thrown.exchange(true))
    {
      throw std::runtime_error{"poll_end_throws"};
    }
  };

  Backend::start(backend_options);

  // Invalid format string — causes _populate_formatted_log_message to throw,
  // which calls _notify_error (which also throws). Must not cause an infinite loop.
  LOG_INFO(logger, "{\nwtf\n}");

  // Valid messages that must still be processed despite all the earlier errors.
  LOG_INFO(logger, "{}", "valid_message_after_error");

  LOG_INFO(logger, "Format {:>321.}", 321.3);

  LOG_INFO(logger, "{}", "final_valid_message");

  logger->flush_log();

  Frontend::remove_logger_blocking(logger);
  Backend::stop();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  // The valid messages must appear in the log file — proving the backend recovered.
  REQUIRE(quill::testing::file_contains(file_contents, "valid_message_after_error"));
  REQUIRE(quill::testing::file_contains(file_contents, "final_valid_message"));

  // The format-error message should still be written (the error was in the notifier, not in
  // the error string construction).
  REQUIRE(quill::testing::file_contains(file_contents, "[Could not format log statement."));

  // Poll hooks should have fired
  REQUIRE(on_poll_begin_thrown.load());
  REQUIRE(on_poll_end_thrown.load());

  testing::remove_file(filename);
#endif
}
