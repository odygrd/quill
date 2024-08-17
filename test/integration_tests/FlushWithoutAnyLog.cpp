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

/***/
TEST_CASE("flush_without_any_log")
{
  static constexpr char const* filename = "flush_without_any_log.log";
  static std::string const logger_name = "logger";

  // Start the logging backend thread
  Backend::start();

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

  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));
  std::atomic<bool> is_flush_done = false;

  std::thread watcher(
    [&is_flush_done]()
    {
      uint32_t try_count = 2000;
      while (!is_flush_done.load() && try_count--)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      REQUIRE_EQ(is_flush_done.load(), true);
    });

  logger->flush_log();
  is_flush_done = true;

  watcher.join();
  Backend::stop();

  testing::remove_file(filename);
}