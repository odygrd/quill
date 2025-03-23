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
  static constexpr quill::QueueType queue_type = quill::QueueType::UnboundedBlocking;
  static constexpr size_t initial_queue_capacity = 131'072;
  static constexpr uint32_t blocking_queue_retry_interval_ns = 800;
  static constexpr size_t unbounded_queue_max_capacity = std::numeric_limits<size_t>::max();
  static constexpr quill::HugePagesPolicy huge_pages_policy = quill::HugePagesPolicy::Never;
};

using CustomFrontend = FrontendImpl<CustomFrontendOptions>;
using CustomLogger = LoggerImpl<CustomFrontendOptions>;

TEST_CASE("unbounded_unlimited_queue")
{
  static constexpr char const* filename = "unbounded_unlimited_queue.log";
  static std::string const logger_name = "logger";
  static constexpr size_t number_of_messages = 1000;

  // Start the logging backend thread
  Backend::start();

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

  // Log a string
  for (size_t i = 0; i < number_of_messages; ++i)
  {
    std::string v{"Lorem ipsum dolor sit amet, consectetur "};
    v += std::to_string(i);

    LOG_INFO(logger, "Logging int: {}, int: {}, string: {}, char: {}", i, i * 10, v, v.c_str());
  }

#ifdef QUILL_ENABLE_EXTENSIVE_TESTS
  // log a very large string
  std::string const very_large(std::numeric_limits<uint32_t>::max(), 'A');
  LOG_INFO(logger, "Very large string {}", very_large);
#endif

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

#ifdef QUILL_ENABLE_EXTENSIVE_TESTS
  REQUIRE_EQ(file_contents.size(), number_of_messages + 1);
#else
  REQUIRE_EQ(file_contents.size(), number_of_messages);
#endif

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Logging int: 0, int: 0, string: Lorem ipsum dolor sit amet, consectetur 0, char: Lorem ipsum dolor sit amet, consectetur 0"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Logging int: 999, int: 9990, string: Lorem ipsum dolor sit amet, consectetur 999, char: Lorem ipsum dolor sit amet, consectetur 999"}));

#ifdef QUILL_ENABLE_EXTENSIVE_TESTS
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Very large string"}));
#endif

  testing::remove_file(filename);
}
