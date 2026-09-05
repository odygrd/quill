#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace quill;

struct SmallBoundedBlockingFrontendOptions : quill::FrontendOptions
{
  static constexpr quill::QueueType queue_type = quill::QueueType::BoundedBlocking;
  static constexpr size_t initial_queue_capacity = 4096;
};

using SmallBoundedBlockingFrontend = FrontendImpl<SmallBoundedBlockingFrontendOptions>;
using SmallBoundedBlockingLogger = LoggerImpl<SmallBoundedBlockingFrontendOptions>;

class RemovalFlushErrorSink final : public Sink
{
public:
  explicit RemovalFlushErrorSink(std::string error) : _error(std::move(error)) {}

  void write_log(MacroMetadata const*, uint64_t, std::string_view, std::string_view,
                 std::string const&, std::string_view, LogLevel, std::string_view, std::string_view,
                 std::vector<std::pair<std::string, std::string>> const*, std::string_view, std::string_view) override
  {
  }

  void flush_sink() override {}

  void flush_sink(SinkFlushReason reason) override
  {
    // Limit failures so a regression fails assertions instead of hanging the test.
    if (reason == SinkFlushReason::Explicit && ++flush_attempts <= 4)
    {
      QUILL_THROW(QuillError{_error});
    }
  }

  size_t flush_attempts{0};

private:
  std::string _error;
};

/***/
TEST_CASE("remove_logger_blocking")
{
  Frontend::remove_logger_blocking(nullptr, 0);

  static constexpr size_t number_of_messages = 10;
  static constexpr char const* filename = "remove_logger_blocking.log";
  static std::string const logger_name = "logger";

  BackendOptions options;
  options.sink_min_flush_interval = std::chrono::hours{24};
  for (size_t iter = 0; iter < 5; ++iter)
  {
    // Start the logging backend thread
    Backend::start(options);

    // Set writing logging to a file
    auto file_sink = Frontend::create_or_get_sink<FileSink>(
      filename,
      []()
      {
        FileSinkConfig cfg;
        cfg.set_open_mode('w');

        // For this test only we use the default buffer size, it should not make any difference it
        // is just for testing the default behaviour and code coverage
        cfg.set_write_buffer_size(0);

        return cfg;
      }(),
      FileEventNotifier{});

    // We create a new logger each iteration here with the iter in the format pattern for testing
    std::string const iter_str = std::to_string(iter) + "_ITER";

    Logger* logger = Frontend::create_or_get_logger(
      logger_name, file_sink, quill::PatternFormatterOptions{iter_str + " %(message)"});

    for (size_t i = 0; i < number_of_messages; ++i)
    {
      LOG_INFO(logger, "This is message {}", i);
    }

    // Removal must flush even while this scope retains the sink and keeps the file open.
    Frontend::remove_logger_blocking(logger, 0);

    // Read file and check
    std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
    REQUIRE_EQ(file_contents.size(), number_of_messages);

    for (size_t i = 0; i < number_of_messages; ++i)
    {
      std::string expected_string = iter_str + " This is message " + std::to_string(i);
      REQUIRE(quill::testing::file_contains(file_contents, expected_string));
    }
  }

  char const* final_filename = "remove_logger_final_flush.log";
  FileSinkConfig final_config;
  final_config.set_open_mode('w');
  auto retained_sink = Frontend::create_sink<FileSink>(final_filename, final_config);
  auto* final_logger = Frontend::create_logger("remove_logger_final_flush", retained_sink);
  LOG_INFO(final_logger, "pending final message");
  Frontend::remove_logger(final_logger);
  Backend::stop();
  CHECK(testing::file_contains(testing::file_contents(final_filename), "pending final message"));
  retained_sink.reset();
  testing::remove_file(final_filename);

#if !defined(QUILL_NO_EXCEPTIONS)
  char const* diagnostic_filename = "remove_logger_flush_diagnostics.log";
  auto diagnostic_sink = Frontend::create_sink<FileSink>(diagnostic_filename, final_config);
  auto* diagnostic_logger = Frontend::create_logger("remove_logger_flush_diagnostics", diagnostic_sink);
  options.error_notifier = [diagnostic_logger](std::string const& error)
  { LOG_ERROR(diagnostic_logger, "{}", error); };

  auto first_sink = std::make_shared<RemovalFlushErrorSink>("first removal flush failed");
  auto second_sink = std::make_shared<RemovalFlushErrorSink>("second removal flush failed");
  auto* failing_logger = Frontend::create_logger(
    "remove_logger_failing_flush", std::vector<std::shared_ptr<Sink>>{first_sink, second_sink});

  Backend::start(options);
  LOG_INFO(failing_logger, "buffered record");
  Frontend::remove_logger_blocking(failing_logger);
  CHECK_EQ(first_sink->flush_attempts, 1);
  CHECK_EQ(second_sink->flush_attempts, 1);
  Frontend::remove_logger_blocking(diagnostic_logger);
  Backend::stop();

  auto const diagnostics = testing::file_contents(diagnostic_filename);
  REQUIRE_EQ(diagnostics.size(), 2);
  CHECK(testing::file_contains(diagnostics, "first removal flush failed"));
  CHECK(testing::file_contains(diagnostics, "second removal flush failed"));
  diagnostic_sink.reset();
  testing::remove_file(diagnostic_filename);
#endif

  // Remove the file
  testing::remove_file(filename);
}
