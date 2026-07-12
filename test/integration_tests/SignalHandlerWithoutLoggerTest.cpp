#include "doctest/doctest.h"

#include "quill/Backend.h"

#include <csignal>
#include <cstdlib>

#if !defined(_WIN32)
  #include <sys/wait.h>
  #include <unistd.h>
#endif

using namespace quill;

TEST_CASE("signal_handler_sigterm_without_logger")
{
#if defined(_WIN32) || !defined(QUILL_ENABLE_EXTENSIVE_TESTS) ||                                   \
  QUILL_HAS_FEATURE(thread_sanitizer) || defined(__SANITIZE_THREAD__)
  // This fork-based signal test is opt-in and unsupported under TSan.
  return;
#else
  pid_t const child_pid = fork();
  REQUIRE_NE(child_pid, -1);

  if (child_pid == 0)
  {
    SignalHandlerOptions signal_handler_options{};
    signal_handler_options.catchable_signals = {SIGTERM};
    signal_handler_options.timeout_seconds = 0;

    Backend::start<FrontendOptions>(BackendOptions{}, signal_handler_options);
    std::raise(SIGTERM);
    std::_Exit(EXIT_FAILURE);
  }

  int child_status{0};
  REQUIRE_EQ(waitpid(child_pid, &child_status, 0), child_pid);
  REQUIRE(WIFEXITED(child_status));
  REQUIRE_EQ(WEXITSTATUS(child_status), EXIT_SUCCESS);
#endif
}
