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
#endif
} // namespace

/***/
TEST_CASE("backend_start_signal_registration_failure_restores_caller_signal_state")
{
  SignalStateRestorer restore_state;

#if defined(_WIN32)
  auto const previous_sigterm_handler = std::signal(SIGTERM, test_sigterm_handler);
  REQUIRE_NE(previous_sigterm_handler, SIG_ERR);

  std::vector<int> const catchable_signals{SIGTERM, 9999};
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
#endif

#if defined(_WIN32)
  REQUIRE_THROWS_AS(init_signal_handler<FrontendOptions>(catchable_signals), QuillError);

  auto const current_sigterm_handler = std::signal(SIGTERM, test_sigterm_handler);
  CHECK_EQ(reinterpret_cast<void*>(current_sigterm_handler), reinterpret_cast<void*>(test_sigterm_handler));

  std::signal(SIGTERM, previous_sigterm_handler);
#else
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
}

#if defined(_WIN32)
/***/
TEST_CASE("windows_console_signal_handler_returns_false_when_unhandled")
{
  auto& ctx = detail::SignalHandlerContext::instance();
  uint32_t const previous_backend_thread_id = ctx.backend_thread_id.load();
  uint32_t const current_thread_id = detail::get_thread_id();
  uint32_t const different_backend_thread_id =
    (current_thread_id == std::numeric_limits<uint32_t>::max()) ? (current_thread_id - 1u)
                                                                : (current_thread_id + 1u);

  ctx.backend_thread_id.store(different_backend_thread_id);

  CHECK_EQ(detail::on_console_signal<FrontendOptions>(CTRL_C_EVENT), FALSE);
  CHECK_EQ(detail::on_console_signal<FrontendOptions>(CTRL_CLOSE_EVENT), FALSE);

  ctx.backend_thread_id.store(previous_backend_thread_id);
}
#endif
