#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <cstddef>
#include <string>
#include <vector>

using namespace quill;

namespace
{
struct SmallQueueFrontendOptions : FrontendOptions
{
  static constexpr size_t initial_queue_capacity = 1024;
};
} // namespace

/***/
TEST_CASE("frontend_preallocate_after_queue_growth")
{
  static constexpr char const* filename = "frontend_preallocate_after_queue_growth.log";
  static std::string const logger_name = "frontend_preallocate_after_queue_growth_logger";
  static constexpr size_t number_of_rounds = 100;
  static constexpr size_t records_per_round = 20;

  using SmallQueueFrontend = FrontendImpl<SmallQueueFrontendOptions>;

  Backend::start();

  auto file_sink = SmallQueueFrontend::create_or_get_sink<FileSink>(filename,
                                                                    []()
                                                                    {
                                                                      FileSinkConfig cfg;
                                                                      cfg.set_open_mode('w');
                                                                      return cfg;
                                                                    }());

  auto* logger = SmallQueueFrontend::create_or_get_logger(logger_name, std::move(file_sink));

  // preallocate() is documented for thread start-up but can be called at any time, e.g. at the
  // start of each job of a thread pool. It used to read the capacity of the queue node that the
  // backend reads from, which the backend frees once the queue has grown or shrunk; sanitizer
  // builds report that read.
  std::string const payload(300, 'x');

  for (size_t round = 0; round < number_of_rounds; ++round)
  {
    for (size_t i = 0; i < records_per_round; ++i)
    {
      LOG_INFO(logger, "round {} record {} {}", round, i, payload);
    }

    SmallQueueFrontend::preallocate();

    // Gives the queue a new node, so that the backend frees the one it is reading from
    SmallQueueFrontend::shrink_thread_local_queue(SmallQueueFrontendOptions::initial_queue_capacity);
  }

  logger->flush_log();
  SmallQueueFrontend::remove_logger(logger);
  Backend::stop();

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), number_of_rounds * records_per_round);

  testing::remove_file(filename);
}
