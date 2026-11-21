#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/sinks/FileSink.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <thread>

#if !defined(_WIN32)
  #include <pthread.h>
  #include <sys/wait.h>
  #include <unistd.h>
#endif

using namespace quill;

TEST_CASE("signal_handler_nested_signal")
{
#if defined(_WIN32) || !defined(QUILL_ENABLE_EXTENSIVE_TESTS) ||                                   \
  QUILL_HAS_FEATURE(thread_sanitizer) || defined(__SANITIZE_THREAD__)
  // This fork-based signal test is opt-in and unsupported under TSan.
  return;
#else
  static constexpr char const* filename = "signal_handler_nested_signal.log";
  static std::atomic<bool> backend_is_writing{false};
  static std::atomic<bool> release_backend{false};

  pid_t const child_pid = fork();
  REQUIRE_NE(child_pid, -1);

  if (child_pid == 0)
  {
    SignalHandlerOptions signal_handler_options{};
    signal_handler_options.catchable_signals = {SIGUSR1, SIGINT};
    signal_handler_options.timeout_seconds = 0;

    Backend::start<FrontendOptions>(BackendOptions{}, signal_handler_options);

    // Holds the backend inside the write of the handler's first record, so the handler stays in
    // flush_log() until the second signal has interrupted it
    FileEventNotifier file_event_notifier;
    file_event_notifier.before_write = [](std::string_view message)
    {
      backend_is_writing.store(true);

      while (!release_backend.load())
      {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
      }

      return std::string{message};
    };

    FileSinkConfig file_sink_config;
    file_sink_config.set_open_mode('w');

    auto file_sink = Frontend::create_or_get_sink<FileSink>(filename, file_sink_config, file_event_notifier);
    Frontend::create_or_get_logger("signal_handler_nested_signal_logger", std::move(file_sink));

    pthread_t const handler_thread = pthread_self();

    std::thread second_signal_thread{[handler_thread]()
                                     {
                                       while (!backend_is_writing.load())
                                       {
                                         std::this_thread::sleep_for(std::chrono::milliseconds{1});
                                       }

                                       // Deliver a second handled signal to the thread that is running the handler
                                       pthread_kill(handler_thread, SIGINT);
                                       std::this_thread::sleep_for(std::chrono::milliseconds{200});

                                       release_backend.store(true);
                                     }};
    second_signal_thread.detach();

    std::raise(SIGUSR1);
    std::_Exit(EXIT_FAILURE);
  }

  // The handler used to park the thread it was already running on when the second signal
  // arrived, so the child never terminated
  int child_status{0};
  bool child_terminated{false};

  for (int attempt = 0; attempt < 3000; ++attempt)
  {
    pid_t const waited_pid = waitpid(child_pid, &child_status, WNOHANG);
    REQUIRE_NE(waited_pid, -1);

    if (waited_pid == child_pid)
    {
      child_terminated = true;
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds{10});
  }

  if (!child_terminated)
  {
    kill(child_pid, SIGKILL);
    waitpid(child_pid, &child_status, 0);
  }

  std::remove(filename);

  REQUIRE(child_terminated);
  REQUIRE(WIFSIGNALED(child_status));
  REQUIRE_EQ(WTERMSIG(child_status), SIGUSR1);
#endif
}
