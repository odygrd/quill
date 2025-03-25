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

// Define custom Frontend Options
struct CustomFrontendOptions
{
  static constexpr quill::QueueType queue_type = quill::QueueType::UnboundedBlocking;
  static constexpr size_t initial_queue_capacity = 16 * 1024; // 16 KiB
  static constexpr uint32_t blocking_queue_retry_interval_ns = 800;
  static constexpr size_t unbounded_queue_max_capacity = 2ull * 1024 * 1024 * 1024; // 2 GiB
  static constexpr quill::HugePagesPolicy huge_pages_policy = quill::HugePagesPolicy::Never;
};

using CustomFrontend = FrontendImpl<CustomFrontendOptions>;
using CustomLogger = LoggerImpl<CustomFrontendOptions>;

/***/
TEST_CASE("shrink_thread_local_queue")
{
  static constexpr size_t number_of_messages = 5000;
  static constexpr size_t iterations = 6;
  static constexpr char const* filename = "shrink_thread_local_queue.log";
  static std::string const logger_name = "logger";

  Backend::start();

  // just for testing - call before logging anything
  CustomFrontend::shrink_thread_local_queue(8 * 1024);
  REQUIRE_EQ(CustomFrontend::get_thread_local_queue_capacity(), 8 * 1024);

  // Set writing logging to a file
  auto file_sink = CustomFrontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');

      // For this test only we use the default buffer size, it should not make any difference it is just for testing the default behaviour and code coverage
      cfg.set_write_buffer_size(0);

      return cfg;
    }(),
    FileEventNotifier{});

  CustomLogger* logger = CustomFrontend::create_or_get_logger(logger_name, std::move(file_sink));

  REQUIRE_EQ(CustomFrontend::get_thread_local_queue_capacity(), 8 * 1024);

  size_t cnt{0};
  for (size_t iter = 0; iter < iterations; ++iter)
  {
    for (size_t i = 0; i < number_of_messages; ++i)
    {
      LOG_INFO(logger, "This is message {}", cnt++);
    }

    CustomFrontend::shrink_thread_local_queue(16 * 1024);
    REQUIRE_EQ(CustomFrontend::get_thread_local_queue_capacity(), 16 * 1024);

    if (iter % 2 == 0)
    {
      // flush the log so that the backend goes idle and also backend shrink is tested
      logger->flush_log();
      std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
  }

  logger->flush_log();
  CustomFrontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), number_of_messages * iterations);

  for (size_t i = 0; i < number_of_messages * iterations; ++i)
  {
    std::string expected_string = logger_name + "       This is message " + std::to_string(i);
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));
  }

  testing::remove_file(filename);
}
