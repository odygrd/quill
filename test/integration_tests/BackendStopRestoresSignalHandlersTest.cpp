#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"

#include <csignal>
#include <vector>

#if !defined(_WIN32)
  #include <signal.h>
#endif

using namespace quill;

#if !defined(_WIN32)
namespace
{
void user_sigterm_handler(int, siginfo_t*, void*) {}
} // namespace
#endif

/***/
TEST_CASE("backend_stop_restores_signal_handlers_installed_before_start")
{
#if defined(_WIN32)
  return;
#else
  // A handler installed by the application with sigaction() flags std::signal() cannot express
  struct sigaction user_action
  {
  };
  user_action.sa_sigaction = user_sigterm_handler;
  sigemptyset(&user_action.sa_mask);
  user_action.sa_flags = SA_SIGINFO | SA_RESTART;

  struct sigaction original_action
  {
  };
  REQUIRE_EQ(sigaction(SIGTERM, &user_action, &original_action), 0);

  SignalHandlerOptions signal_handler_options;
  signal_handler_options.catchable_signals = std::vector<int>{SIGTERM};
  Backend::start<FrontendOptions>(BackendOptions{}, signal_handler_options);

  struct sigaction quill_action
  {
  };
  REQUIRE_EQ(sigaction(SIGTERM, nullptr, &quill_action), 0);
  REQUIRE_NE(reinterpret_cast<void*>(quill_action.sa_sigaction), reinterpret_cast<void*>(user_sigterm_handler));

  Backend::stop();

  struct sigaction restored_action
  {
  };
  REQUIRE_EQ(sigaction(SIGTERM, nullptr, &restored_action), 0);
  REQUIRE_EQ(reinterpret_cast<void*>(restored_action.sa_sigaction), reinterpret_cast<void*>(user_sigterm_handler));
  REQUIRE_NE(restored_action.sa_flags & SA_SIGINFO, 0);

  sigaction(SIGTERM, &original_action, nullptr);
#endif
}
