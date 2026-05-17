#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

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

/***/
TEST_CASE("remove_logger_blocking_retries_when_frontend_queue_is_full")
{
  static constexpr char const* filename = "remove_logger_blocking_queue_full.log";
  static std::string const logger_name = "queue_full_logger";

  Backend::start();

  auto file_sink = SmallBoundedBlockingFrontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  SmallBoundedBlockingLogger* logger =
    SmallBoundedBlockingFrontend::create_or_get_logger(logger_name, std::move(file_sink));

  std::string const payload(1024, 'q');

  for (size_t i = 0; i < 32; ++i)
  {
    LOG_INFO(logger, "queue fill {} {}", i, payload);
  }

  SmallBoundedBlockingFrontend::remove_logger_blocking(logger, 0);
  Backend::stop();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE(!file_contents.empty());
  REQUIRE(quill::testing::file_contains(file_contents, logger_name + " queue fill 0"));
  REQUIRE(quill::testing::file_contains(file_contents, logger_name + " queue fill 31"));

  testing::remove_file(filename);
}
