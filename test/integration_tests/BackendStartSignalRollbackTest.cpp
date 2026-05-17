#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/backend/SignalHandler.h"

#include <csignal>
#include <limits>
#include <mutex>

#if !defined(_WIN32)
  #include <pthread.h>
#endif

using namespace quill;

namespace
{
#if !defined(QUILL_NO_EXCEPTIONS)
void test_sigterm_handler(int) {}

  #if defined(_WIN32)
struct SignalStateRestorer
{
  SignalStateRestorer()
  {
    original_sigterm_handler = std::signal(SIGTERM, test_sigterm_handler);
    REQUIRE_NE(original_sigterm_handler, SIG_ERR);

    auto const previous_test_sigterm_handler = std::signal(SIGTERM, original_sigterm_handler);
    REQUIRE_EQ(reinterpret_cast<void*>(previous_test_sigterm_handler),
               reinterpret_cast<void*>(test_sigterm_handler));
  }

  ~SignalStateRestorer()
  {
    std::signal(SIGTERM, original_sigterm_handler);

    auto& ctx = detail::SignalHandlerContext::instance();
    std::lock_guard<std::mutex> const lock{ctx.signal_handlers_mutex};
    ctx.registered_signal_handlers.clear();
    ctx.previous_signal_handlers.clear();
  }

  detail::signal_handler_t original_sigterm_handler{SIG_DFL};
};
  #else
struct SignalStateRestorer
{
  SignalStateRestorer()
  {
    REQUIRE_EQ(sigaction(SIGTERM, nullptr, &original_sigterm_action), 0);
    REQUIRE_EQ(sigaction(SIGALRM, nullptr, &original_sigalrm_action), 0);
    REQUIRE_EQ(pthread_sigmask(SIG_SETMASK, nullptr, &original_mask), 0);
  }

  ~SignalStateRestorer()
  {
    sigaction(SIGTERM, &original_sigterm_action, nullptr);
    sigaction(SIGALRM, &original_sigalrm_action, nullptr);
    pthread_sigmask(SIG_SETMASK, &original_mask, nullptr);

    auto& ctx = detail::SignalHandlerContext::instance();
    std::lock_guard<std::mutex> const lock{ctx.signal_handlers_mutex};
    ctx.registered_signal_handlers.clear();
  }

  struct sigaction original_sigterm_action{};
  struct sigaction original_sigalrm_action{};
  sigset_t original_mask{};
};
  #endif // !defined(_WIN32)
#endif   // !defined(QUILL_NO_EXCEPTIONS)
} // namespace

/***/
TEST_CASE("backend_start_signal_registration_failure_restores_caller_signal_state")
{
#if !defined(QUILL_NO_EXCEPTIONS)
  SignalStateRestorer restore_state;

  #if defined(_WIN32)
  auto const previous_sigterm_handler = std::signal(SIGTERM, test_sigterm_handler);
  REQUIRE_NE(previous_sigterm_handler, SIG_ERR);

  std::vector<int> const catchable_signals{SIGTERM, 9999};

  REQUIRE_THROWS_AS(init_signal_handler<FrontendOptions>(catchable_signals), QuillError);

  auto const current_sigterm_handler = std::signal(SIGTERM, test_sigterm_handler);
  CHECK_EQ(reinterpret_cast<void*>(current_sigterm_handler), reinterpret_cast<void*>(test_sigterm_handler));

  std::signal(SIGTERM, previous_sigterm_handler);
  #else
  sigset_t baseline_mask = restore_state.original_mask;
  REQUIRE_EQ(sigdelset(&baseline_mask, SIGTERM), 0);
  REQUIRE_EQ(sigdelset(&baseline_mask, SIGINT), 0);
  REQUIRE_EQ(sigdelset(&baseline_mask, SIGABRT), 0);
  REQUIRE_EQ(pthread_sigmask(SIG_SETMASK, &baseline_mask, nullptr), 0);

  auto const previous_sigterm_handler = std::signal(SIGTERM, test_sigterm_handler);
  REQUIRE_NE(previous_sigterm_handler, SIG_ERR);

  SignalHandlerOptions signal_handler_options;
  signal_handler_options.catchable_signals = {SIGTERM, SIGALRM};

  // Backend::start() propagates signal-registration failures to the caller.
  REQUIRE_THROWS_AS(Backend::start<FrontendOptions>(BackendOptions{}, signal_handler_options), QuillError);

  sigset_t current_mask;
  REQUIRE_EQ(pthread_sigmask(SIG_SETMASK, nullptr, &current_mask), 0);

  CHECK_EQ(sigismember(&current_mask, SIGTERM), 0);
  CHECK_EQ(sigismember(&current_mask, SIGINT), 0);
  CHECK_EQ(sigismember(&current_mask, SIGABRT), 0);

  auto const current_sigterm_handler = std::signal(SIGTERM, test_sigterm_handler);
  CHECK_EQ(reinterpret_cast<void*>(current_sigterm_handler), reinterpret_cast<void*>(test_sigterm_handler));

  std::signal(SIGTERM, previous_sigterm_handler);
  #endif
#endif
}
