#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <atomic>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

using namespace quill;

/***/
TEST_CASE("file_sink_concurrent_stream_flush")
{
  static constexpr size_t number_of_messages = 20000;
  static constexpr char const* buffered_filename = "file_sink_concurrent_stream_flush_buffered.log";
  static constexpr char const* unbuffered_filename =
    "file_sink_concurrent_stream_flush_unbuffered.log";

  Backend::start();

  // One sink keeps the records in its write buffer. The other one writes them through the stream
  // of the file, which the other thread of this test flushes as well
  auto buffered_sink = Frontend::create_or_get_sink<FileSink>(
    buffered_filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  auto unbuffered_sink = Frontend::create_or_get_sink<FileSink>(
    unbuffered_filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      cfg.set_write_buffer_size(0);
      return cfg;
    }(),
    FileEventNotifier{});

  PatternFormatterOptions const pattern_formatter_options{"%(message)"};

  Logger* buffered_logger = Frontend::create_or_get_logger(
    "file_sink_concurrent_stream_flush_buffered", std::move(buffered_sink), pattern_formatter_options);

  Logger* unbuffered_logger = Frontend::create_or_get_logger(
    "file_sink_concurrent_stream_flush_unbuffered", std::move(unbuffered_sink), pattern_formatter_options);

  // An application, or a library that it uses, can flush every open stream at any time
  std::atomic<bool> stop_flushing{false};
  std::thread flushing_thread{[&stop_flushing]()
                              {
                                while (!stop_flushing.load(std::memory_order_relaxed))
                                {
                                  std::fflush(nullptr);
                                }
                              }};

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    LOG_INFO(buffered_logger, "message {}", i);
    LOG_INFO(unbuffered_logger, "message {}", i);
  }

  buffered_logger->flush_log();
  unbuffered_logger->flush_log();

  stop_flushing.store(true, std::memory_order_relaxed);
  flushing_thread.join();

  Frontend::remove_logger(buffered_logger);
  Frontend::remove_logger(unbuffered_logger);
  Backend::stop();

  // Every record is in each file once, in the order it was logged
  for (char const* filename : {buffered_filename, unbuffered_filename})
  {
    std::vector<std::string> const file_contents = testing::file_contents(filename);
    REQUIRE_EQ(file_contents.size(), number_of_messages);

    for (size_t i = 0; i < number_of_messages; ++i)
    {
      REQUIRE_EQ(file_contents[i], "message " + std::to_string(i));
    }

    testing::remove_file(filename);
  }
}
