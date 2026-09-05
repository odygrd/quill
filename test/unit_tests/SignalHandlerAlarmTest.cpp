#include "doctest/doctest.h"

#include "quill/backend/SignalHandler.h"

#if !defined(_WIN32)
  #include <sys/resource.h>
  #include <sys/wait.h>
  #include <unistd.h>
#endif

TEST_CASE("signal_alarm_unblocks_original_signal")
{
#if defined(_WIN32) || QUILL_HAS_FEATURE(thread_sanitizer) || defined(__SANITIZE_THREAD__)
  return;
#else
  for (int const original_signal : {SIGTERM, SIGABRT, SIGSEGV})
  {
    pid_t const child = fork();
    REQUIRE_NE(child, -1);
    if (child == 0)
    {
      // Avoid leaving core files in the test directory. Termination must still use the
      // original default signal action, rather than an exit code or a different signal.
      rlimit const core_limit{0, 0};
      setrlimit(RLIMIT_CORE, &core_limit);
      sigset_t blocked_signals;
      sigemptyset(&blocked_signals);
      sigaddset(&blocked_signals, original_signal);
      pthread_sigmask(SIG_BLOCK, &blocked_signals, nullptr);
      quill::detail::SignalHandlerContext::instance().signal_number.store(original_signal);
      quill::detail::on_alarm(SIGALRM);
      std::_Exit(1);
    }

    int status{0};
    REQUIRE_EQ(waitpid(child, &status, 0), child);
    CHECK(WIFSIGNALED(status));
    CHECK_EQ(WTERMSIG(status), original_signal);
  }
#endif
}
