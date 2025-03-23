#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <string>
#include <vector>

using namespace quill;

// Define custom Frontend Options
struct CustomFrontendOptions
{
  static constexpr quill::QueueType queue_type = quill::QueueType::BoundedDropping;
  static constexpr size_t initial_queue_capacity = 1024;
  static constexpr uint32_t blocking_queue_retry_interval_ns = 800;
  static constexpr size_t unbounded_queue_max_capacity = 2ull * 1024u * 1024u * 1024u;
  static constexpr quill::HugePagesPolicy huge_pages_policy = quill::HugePagesPolicy::Never;
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
  { dropped_messages = error_message; };

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

  // Wake up the backend thread to process
  Backend::notify();

  do
  {
    // Keep checking the file for at least one log output
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  } while (testing::file_contents(filename).empty());

  // Log another message using the conditional arg size cache after LOG_INFO failed dropping
  // messages// This is important to log as it could catch triggering an assertion
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
  std::string search_end = " log";
  uint64_t dropped_messages_num{0};

  // Find the starting and ending positions
  std::size_t start_pos = dropped_messages.find(search_start);
  std::size_t end_pos = dropped_messages.find(search_end);

  if (start_pos != std::string::npos && end_pos != std::string::npos)
  {
    // Adjust start_pos to get the number after "Dropped "
    start_pos += search_start.length();

    // Extract the substring between "Dropped " and " log"
    std::string number_str = dropped_messages.substr(start_pos, end_pos - start_pos);
    dropped_messages_num = std::stoul(number_str);
  }

  REQUIRE_GE(dropped_messages_num, 1);
  REQUIRE_LE(dropped_messages_num, number_of_messages);

  testing::remove_file(filename);
}
