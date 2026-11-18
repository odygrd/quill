#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/core/QuillError.h"
#include "quill/sinks/NullSink.h"

using namespace quill;

namespace
{
struct BoundedDroppingFrontendOptions : FrontendOptions
{
  static constexpr QueueType queue_type = QueueType::BoundedDropping;
};
} // namespace

/***/
TEST_CASE("frontend_options_queue_mismatch")
{
#if !defined(QUILL_NO_EXCEPTIONS)
  // Querying the capacity creates this thread's queue with the default FrontendOptions, before
  // any logger exists. A logger with another queue type would reinterpret that queue.
  REQUIRE_EQ(Frontend::get_thread_local_queue_capacity(), FrontendOptions::initial_queue_capacity);

  using BoundedFrontend = FrontendImpl<BoundedDroppingFrontendOptions>;
  auto sink = Frontend::create_or_get_sink<NullSink>("frontend_options_queue_mismatch_sink");

  REQUIRE_THROWS_AS(BoundedFrontend::create_or_get_logger("frontend_options_queue_mismatch_logger", sink),
                    QuillError);
  REQUIRE_THROWS_AS(BoundedFrontend::preallocate(), QuillError);
#endif
}
