#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace quill;

/***/
TEST_CASE("backend_spin_wait")
{
  static constexpr char const* filename = "backend_spin_wait.log";
  static constexpr size_t number_of_threads = 4;
  static constexpr size_t messages_per_thread = 50;

  // The backend busy-waits and pauses for 1 ms each time it finds the queues empty, so most
  // records below are logged while it waits. Every record must still reach the sink
  BackendOptions backend_options;
  backend_options.sleep_duration = std::chrono::nanoseconds{0};
  backend_options.spin_wait_duration = std::chrono::milliseconds{1};
  Backend::start(backend_options);

  auto file_sink = Frontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger(
    "logger_backend_spin_wait", std::move(file_sink),
    PatternFormatterOptions{"%(log_level) %(message)", "%H:%M:%S.%Qns", Timezone::GmtTime, false});

  std::vector<std::thread> threads;

  for (size_t thread_index = 0; thread_index < number_of_threads; ++thread_index)
  {
    threads.emplace_back(
      [logger, thread_index]()
      {
        for (size_t i = 0; i < messages_per_thread; ++i)
        {
          LOG_INFO(logger, "Thread {} message {}", thread_index, i);
          std::this_thread::sleep_for(std::chrono::microseconds{100});
        }
      });
  }

  for (auto& thread : threads)
  {
    thread.join();
  }

  logger->flush_log();
  Frontend::remove_logger_blocking(logger);
  Backend::stop();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), number_of_threads * messages_per_thread);

  for (size_t thread_index = 0; thread_index < number_of_threads; ++thread_index)
  {
    REQUIRE(quill::testing::file_contains(
      file_contents, "INFO Thread " + std::to_string(thread_index) + " message 0"));
    REQUIRE(quill::testing::file_contains(file_contents,
                                          "INFO Thread " + std::to_string(thread_index) +
                                            " message " + std::to_string(messages_per_thread - 1)));
  }

  testing::remove_file(filename);
}
