#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"

#include <csignal>
#include <vector>

#if !defined(_WIN32)
  #include <signal.h>
#endif

using namespace quill;

namespace
{
#if defined(_WIN32)
int original_filter_calls{0};
LPTOP_LEVEL_EXCEPTION_FILTER filter_replaced_by_application{nullptr};

LONG WINAPI original_filter(EXCEPTION_POINTERS*)
{
  ++original_filter_calls;
  return EXCEPTION_CONTINUE_SEARCH;
}

// Forwards to the filter it replaced, as crash reporters commonly do
LONG WINAPI application_filter(EXCEPTION_POINTERS* exception_pointers)
{
  return filter_replaced_by_application(exception_pointers);
}
#else
void user_sigterm_handler(int, siginfo_t*, void*) {}
void user_sigalrm_handler(int) {}
#endif
} // namespace

/***/
TEST_CASE("backend_stop_restores_signal_handlers_installed_before_start")
{
#if defined(_WIN32)
  auto const previous = ::SetUnhandledExceptionFilter(original_filter);
  Backend::start<FrontendOptions>(BackendOptions{}, SignalHandlerOptions{});
  Backend::stop();
  CHECK_EQ(::SetUnhandledExceptionFilter(original_filter), &original_filter);

  // A filter the application installs after start() stays installed. Through Quill's filter it
  // still reaches the original one, after stop() and after a restart, without forwarding in a loop
  EXCEPTION_RECORD exception_record{};
  exception_record.ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
  EXCEPTION_POINTERS exception_pointers{&exception_record, nullptr};

  Backend::start<FrontendOptions>(BackendOptions{}, SignalHandlerOptions{});
  filter_replaced_by_application = ::SetUnhandledExceptionFilter(application_filter);
  Backend::stop();

  CHECK_EQ(::SetUnhandledExceptionFilter(application_filter), &application_filter);
  CHECK_EQ(application_filter(&exception_pointers), EXCEPTION_CONTINUE_SEARCH);
  CHECK_EQ(original_filter_calls, 1);

  Backend::start<FrontendOptions>(BackendOptions{}, SignalHandlerOptions{});
  CHECK_EQ(::SetUnhandledExceptionFilter(application_filter), &application_filter);
  CHECK_EQ(application_filter(&exception_pointers), EXCEPTION_CONTINUE_SEARCH);
  CHECK_EQ(original_filter_calls, 2);
  Backend::stop();

  REQUIRE_EQ(::SetUnhandledExceptionFilter(previous), &application_filter);
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

  // With no alarm timeout, Quill must leave the application's SIGALRM handler alone
  struct sigaction user_alarm_action
  {
  };
  user_alarm_action.sa_handler = user_sigalrm_handler;
  sigemptyset(&user_alarm_action.sa_mask);

  struct sigaction original_alarm_action
  {
  };
  REQUIRE_EQ(sigaction(SIGALRM, &user_alarm_action, &original_alarm_action), 0);

  SignalHandlerOptions signal_handler_options;
  signal_handler_options.catchable_signals = std::vector<int>{SIGTERM};
  signal_handler_options.timeout_seconds = 0;
  Backend::start<FrontendOptions>(BackendOptions{}, signal_handler_options);

  struct sigaction quill_action
  {
  };
  REQUIRE_EQ(sigaction(SIGTERM, nullptr, &quill_action), 0);
  REQUIRE_NE(reinterpret_cast<void*>(quill_action.sa_sigaction), reinterpret_cast<void*>(user_sigterm_handler));

  struct sigaction alarm_action_after_start
  {
  };
  REQUIRE_EQ(sigaction(SIGALRM, nullptr, &alarm_action_after_start), 0);
  REQUIRE_EQ(reinterpret_cast<void*>(alarm_action_after_start.sa_handler),
             reinterpret_cast<void*>(user_sigalrm_handler));

  Backend::stop();

  struct sigaction restored_action
  {
  };
  REQUIRE_EQ(sigaction(SIGTERM, nullptr, &restored_action), 0);
  REQUIRE_EQ(reinterpret_cast<void*>(restored_action.sa_sigaction), reinterpret_cast<void*>(user_sigterm_handler));
  REQUIRE_NE(restored_action.sa_flags & SA_SIGINFO, 0);

  sigaction(SIGTERM, &original_action, nullptr);
  sigaction(SIGALRM, &original_alarm_action, nullptr);
#endif
}
