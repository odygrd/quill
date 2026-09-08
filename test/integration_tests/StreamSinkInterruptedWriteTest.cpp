#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/StreamSink.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

#if !defined(_WIN32)
  #include <csignal>
  #include <pthread.h>
  #include <unistd.h>

namespace
{
volatile sig_atomic_t signals_received{0};

void interrupt_stream_write(int) { ++signals_received; }

struct InterruptedWriteFrontendOptions : quill::FrontendOptions
{
  static constexpr size_t initial_queue_capacity = 2 * 1024 * 1024;
};
} // namespace
#endif

TEST_CASE("stream_sink_preserves_log_when_pipe_write_is_interrupted")
{
#if !defined(_WIN32)
  using namespace quill;
  using TestFrontend = FrontendImpl<InterruptedWriteFrontendOptions>;

  struct sigaction previous_action
  {
  };
  struct sigaction action
  {
  };
  action.sa_handler = interrupt_stream_write;
  sigemptyset(&action.sa_mask);
  REQUIRE_EQ(sigaction(SIGUSR1, &action, &previous_action), 0);

  int pipe_fds[2];
  REQUIRE_EQ(pipe(pipe_fds), 0);
  FILE* stream = fdopen(pipe_fds[1], "w");
  REQUIRE_NE(stream, nullptr);
  REQUIRE_EQ(setvbuf(stream, nullptr, _IONBF, 0), 0);

  std::atomic<bool> write_started{false};
  std::atomic<bool> polling_finished{false};
  std::atomic<bool> allow_worker_exit{false};
  size_t error_count{0};
  std::string last_error;
  FileEventNotifier notifier;
  notifier.before_write = [&write_started](std::string_view statement)
  {
    write_started.store(true, std::memory_order_release);
    return std::string{statement};
  };

  auto sink = TestFrontend::create_or_get_sink<StreamSink>(
    "stream_sink_interrupted_pipe", "stream_sink_interrupted_pipe", stream, std::nullopt, notifier);
  auto* logger = TestFrontend::create_or_get_logger("stream_sink_interrupted_pipe_logger", sink,
                                                    PatternFormatterOptions{"%(message)"});
  std::string const payload(1024 * 1024, 'x');
  LOG_INFO(logger, "{}", payload);
  LOG_INFO(logger, "after interruption");

  ManualBackendWorker* backend = Backend::acquire_manual_backend_worker();
  std::thread worker(
    [&]()
    {
      BackendOptions options;
      options.log_timestamp_ordering_grace_period = std::chrono::microseconds{0};
      options.error_notifier = [&](std::string const& error)
      {
        ++error_count;
        last_error = error;
      };
      backend->init(options);
      backend->poll();
      TestFrontend::remove_logger(logger);
      backend->shutdown();
      fclose(stream);
      polling_finished.store(true, std::memory_order_release);
      while (!allow_worker_exit.load(std::memory_order_acquire))
      {
        std::this_thread::yield();
      }
    });

  while (!write_started.load(std::memory_order_acquire))
  {
    std::this_thread::yield();
  }

  // Keep the pipe undrained while interrupting its blocked writer. A write may have
  // already transferred a prefix before the next write reports EINTR.
  for (size_t i = 0; i < 10 && !polling_finished.load(std::memory_order_acquire); ++i)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds{2});
    pthread_kill(worker.native_handle(), SIGUSR1);
  }

  std::string received;
  char buffer[4096];
  ssize_t bytes_read = read(pipe_fds[0], buffer, sizeof(buffer));
  while (bytes_read > 0)
  {
    received.append(buffer, static_cast<size_t>(bytes_read));
    bytes_read = read(pipe_fds[0], buffer, sizeof(buffer));
  }

  close(pipe_fds[0]);
  allow_worker_exit.store(true, std::memory_order_release);
  worker.join();
  sigaction(SIGUSR1, &previous_action, nullptr);

  REQUIRE_EQ(bytes_read, 0);
  REQUIRE_GT(signals_received, 0);
  INFO(last_error);
  REQUIRE_EQ(error_count, 0);
  REQUIRE(received == payload + "\nafter interruption\n");
#endif
}
