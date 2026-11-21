#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/core/QuillError.h"
#include "quill/sinks/NullSink.h"

using namespace quill;

#if !defined(QUILL_NO_EXCEPTIONS)
namespace
{
struct BoundedDroppingFrontendOptions : FrontendOptions
{
  static constexpr QueueType queue_type = QueueType::BoundedDropping;
};
} // namespace
#endif

/***/
TEST_CASE("frontend_options_queue_mismatch")
{
#if !defined(QUILL_NO_EXCEPTIONS)
  // Querying the capacity creates this thread's queue with the default FrontendOptions, before
  // any logger exists. A logger with another queue type would reinterpret that queue.
  REQUIRE_EQ(Frontend::get_thread_local_queue_capacity(), FrontendOptions::initial_queue_capacity);

  using BoundedFrontend = FrontendImpl<BoundedDroppingFrontendOptions>;
  auto sink = Frontend::create_or_get_sink<NullSink>("frontend_options_queue_mismatch_sink");

  REQUIRE_THROWS_AS(
    BoundedFrontend::create_or_get_logger("frontend_options_queue_mismatch_logger", sink), QuillError);
  REQUIRE_THROWS_AS(BoundedFrontend::preallocate(), QuillError);

  // The signal handler logs with the FrontendOptions given to Backend::start()
  REQUIRE_THROWS_AS(
    Backend::start<BoundedDroppingFrontendOptions>(BackendOptions{}, SignalHandlerOptions{}), QuillError);
  REQUIRE_FALSE(Backend::is_running());

  #if defined(_WIN32)
  // So does the per-thread CRT signal handler
  REQUIRE_THROWS_AS(init_signal_handler<BoundedDroppingFrontendOptions>({SIGTERM}), QuillError);
  #endif
#endif
}
