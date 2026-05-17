#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/core/QuillError.h"
#include "quill/sinks/NullSink.h"

#include <string>
#include <utility>

using namespace quill;

namespace
{
struct SmallBoundedBlockingFrontendOptions : quill::FrontendOptions
{
  static constexpr quill::QueueType queue_type = quill::QueueType::BoundedBlocking;
  static constexpr size_t initial_queue_capacity = 1024;
};

using SmallBoundedBlockingFrontend = FrontendImpl<SmallBoundedBlockingFrontendOptions>;
using SmallBoundedBlockingLogger = LoggerImpl<SmallBoundedBlockingFrontendOptions>;
} // namespace

#if !defined(QUILL_NO_EXCEPTIONS)
TEST_CASE("bounded_blocking_oversized_message_throws")
{
  static std::string const logger_name = "bounded_blocking_oversized_message_logger";

  Backend::start();

  auto sink = SmallBoundedBlockingFrontend::create_or_get_sink<NullSink>(
    "bounded_blocking_oversized_message_sink");
  SmallBoundedBlockingLogger* logger =
    SmallBoundedBlockingFrontend::create_or_get_logger(logger_name, std::move(sink));

  std::string oversized_message(SmallBoundedBlockingFrontendOptions::initial_queue_capacity + 1u, 'x');

  bool oversized_message_threw{false};
  try
  {
    LOG_INFO(logger, "{}", oversized_message);
  }
  catch (quill::QuillError const&)
  {
    oversized_message_threw = true;
  }

  Frontend::remove_logger(logger);
  Backend::notify();
  Backend::stop();

  REQUIRE(oversized_message_threw);
}
#endif
