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
#if !defined(_WIN32)
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
