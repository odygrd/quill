#include "doctest/doctest.h"

#include "quill/backend/SignalHandler.h"

#include <limits>

using namespace quill;

/***/
TEST_CASE("windows_console_signal_handler_returns_false_when_unhandled")
{
#if defined(_WIN32)
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
#else
  return;
#endif
}
