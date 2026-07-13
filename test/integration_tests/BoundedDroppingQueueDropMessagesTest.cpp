#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <string>
#include <thread>
#include <vector>

using namespace quill;

// Define custom Frontend Options
struct CustomFrontendOptions : quill::FrontendOptions
{
  static constexpr quill::QueueType queue_type = quill::QueueType::BoundedDropping;
  static constexpr size_t initial_queue_capacity = 1024;
};

using CustomFrontend = FrontendImpl<CustomFrontendOptions>;
using CustomLogger = LoggerImpl<CustomFrontendOptions>;

TEST_CASE("bounded_dropping_queue_drop_messages")
{
  static constexpr char const* filename = "bounded_dropping_queue_drop_messages.log";
  static std::string const logger_name = "logger";
  static constexpr size_t number_of_messages = 30u;

  // Start the logging backend thread
  BackendOptions backend_options;

  std::string dropped_messages;
  backend_options.error_notifier = [&dropped_messages](std::string const& error_message)
  { dropped_messages += error_message + "\n"; };

  // sleep long on backend thread to ensure we drop the queue
  backend_options.sleep_duration = std::chrono::seconds{10};
  Backend::start(backend_options);

  auto file_sink = CustomFrontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  CustomLogger* logger = CustomFrontend::create_or_get_logger(logger_name, std::move(file_sink));

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    std::string s;
    s.assign(i, 'A');
    LOG_INFO(logger, "Log something to fulfill the bound queue {} {}", i, s.data());
  }

  // While the queue is still full, issue a control event. It must run on this thread because
  // the failure counter and the queue are per thread context. set_mdc() retries until it is
  // enqueued; the failed attempts must not be counted as dropped events since the event is
  // eventually delivered. A helper thread wakes the backend so the retry loop can complete
  std::thread backend_wakeup_thread(
    []()
    {
      std::this_thread::sleep_for(std::chrono::milliseconds{20});
      Backend::notify();
    });

  logger->set_mdc("mdc_key", "mdc_value");
  backend_wakeup_thread.join();

  // Wake up the backend thread to process
  Backend::notify();

  do
  {
    // Keep checking the file for at least one log output
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  } while (testing::file_contents(filename).empty());

  // Log another message using the conditional arg size cache after LOG_INFO failed dropping
  // messages
  // This is important to log as it could catch triggering an assertion
  char const* t = "test";
  LOG_INFO(logger, "Log something else {}", t);

  // Wake up the backend thread to process
  Backend::notify();

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wake up the backend thread to process
  Backend::notify();

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check the first messages as we can't if some were dropped
  std::vector<std::string> const file_contents = testing::file_contents(filename);

  for (size_t i = 0; i < 10; ++i)
  {
    std::string s;
    s.assign(i, 'A');

    std::string expected_string_1 = "LOG_INFO      " + logger_name +
      "       Log something to fulfill the bound queue " + std::to_string(i) + " " + s;
    REQUIRE(testing::file_contains(file_contents, expected_string_1));
  }

  std::string search_start = "Dropped ";
  std::string search_end = " events";
  uint64_t dropped_messages_num{0};

  // Sum all reported drop counts; the notifier can be invoked more than once
  std::size_t start_pos = dropped_messages.find(search_start);
  while (start_pos != std::string::npos)
  {
    start_pos += search_start.length();
    std::size_t const end_pos = dropped_messages.find(search_end, start_pos);

    if (end_pos == std::string::npos)
    {
      break;
    }

    dropped_messages_num += std::stoul(dropped_messages.substr(start_pos, end_pos - start_pos));
    start_pos = dropped_messages.find(search_start, end_pos);
  }

  // The retried set_mdc() control event must not inflate the count with its failed attempts;
  // only genuinely dropped log records may be counted
  REQUIRE_GE(dropped_messages_num, 1);
  REQUIRE_LE(dropped_messages_num, number_of_messages);

  testing::remove_file(filename);
}
