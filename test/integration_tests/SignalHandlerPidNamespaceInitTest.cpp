#include "doctest/doctest.h"

#include "quill/Backend.h"

#include <csignal>
#include <cstdlib>

#if defined(__linux__) && !defined(__ANDROID__)
  #include <sched.h>
  #include <sys/wait.h>
  #include <unistd.h>
#endif

using namespace quill;

TEST_CASE("signal_handler_pid_namespace_init")
{
#if !defined(__linux__) || defined(__ANDROID__) || !defined(QUILL_ENABLE_EXTENSIVE_TESTS) ||       \
  QUILL_HAS_FEATURE(thread_sanitizer) || defined(__SANITIZE_THREAD__)
  // This fork-based signal test needs Linux PID namespaces, is opt-in and unsupported under TSan.
  return;
#else
  static constexpr int namespaces_unavailable_status = 77;
  static constexpr int unexpected_status = 78;

  for (bool const through_alarm : {false, true})
  {
    pid_t const child_pid = fork();
    REQUIRE_NE(child_pid, -1);

    if (child_pid == 0)
    {
      // The next process forked from here is the init process (PID 1) of a new PID namespace, like
      // the entrypoint of a container
      if (unshare(CLONE_NEWUSER | CLONE_NEWPID) != 0)
      {
        std::_Exit(namespaces_unavailable_status);
      }

      pid_t const init_pid = fork();

      if (init_pid == -1)
      {
        std::_Exit(namespaces_unavailable_status);
      }

      if (init_pid == 0)
      {
        if (getpid() != 1)
        {
          std::_Exit(unexpected_status);
        }

        SignalHandlerOptions signal_handler_options{};
        signal_handler_options.catchable_signals = {SIGTERM};
        signal_handler_options.timeout_seconds = through_alarm ? 1u : 0u;

        Backend::start<FrontendOptions>(BackendOptions{}, signal_handler_options);

        if (through_alarm)
        {
          // The alarm that bounds a handler stuck on SIGTERM re-raises that signal as well
          detail::SignalHandlerContext::instance().signal_number.store(SIGTERM);
          std::raise(SIGALRM);
        }
        else
        {
          std::raise(SIGTERM);
        }

        std::_Exit(EXIT_FAILURE);
      }

      int init_status{0};

      if ((waitpid(init_pid, &init_status, 0) != init_pid) || !WIFEXITED(init_status))
      {
        std::_Exit(unexpected_status);
      }

      std::_Exit(WEXITSTATUS(init_status));
    }

    int child_status{0};
    REQUIRE_EQ(waitpid(child_pid, &child_status, 0), child_pid);
    REQUIRE(WIFEXITED(child_status));

    if (WEXITSTATUS(child_status) == namespaces_unavailable_status)
    {
      // Unprivileged user namespaces are disabled on this machine
      return;
    }

    // The kernel discards the re-raised SIGTERM for an init process, so the handler, or the alarm
    // that bounds it, must end the process itself, with the status a shell reports for a process
    // terminated by SIGTERM
    REQUIRE_EQ(WEXITSTATUS(child_status), 128 + SIGTERM);
  }
#endif
}
