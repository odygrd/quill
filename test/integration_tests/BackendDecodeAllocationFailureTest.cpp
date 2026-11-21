#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/Sink.h"
#include "quill/std/Vector.h"

#include <cstddef>
#include <new>
#include <string>
#include <thread>
#include <vector>

// ThreadSanitizer supplies its own global new/delete definitions, which cannot be replaced.
#if !defined(QUILL_NO_EXCEPTIONS) && !QUILL_HAS_FEATURE(thread_sanitizer) && !defined(__SANITIZE_THREAD__)
namespace
{
// Only the calling thread's next allocation fails; producer allocations are unaffected.
thread_local bool fail_next_allocation{false};
} // namespace

// This executable replaces global operator new to simulate a single allocation failure
// without exhausting memory. Other test executables use their normal allocator.
void* operator new(std::size_t size)
{
  if (fail_next_allocation)
  {
    // Consume the flag before throwing so error reporting and the next poll can allocate.
    fail_next_allocation = false;
    throw std::bad_alloc{};
  }

  return ::operator new(size, std::align_val_t{alignof(std::max_align_t)});
}

void operator delete(void* memory) noexcept
{
  ::operator delete(memory, std::align_val_t{alignof(std::max_align_t)});
}
void operator delete(void* memory, std::size_t) noexcept
{
  ::operator delete(memory, std::align_val_t{alignof(std::max_align_t)});
}
#endif

TEST_CASE("backend_decode_allocation_failure_retries_with_fresh_arguments")
{
#if defined(QUILL_NO_EXCEPTIONS) || QUILL_HAS_FEATURE(thread_sanitizer) || defined(__SANITIZE_THREAD__)
  return;
#elif defined(_ITERATOR_DEBUG_LEVEL) && (_ITERATOR_DEBUG_LEVEL > 0)
  // The MSVC debug STL allocates container proxies inside noexcept moves, where an injected
  // bad_alloc terminates the process before it can reach the backend's error handling
  return;
#else
  class CapturingSink : public quill::Sink
  {
  public:
    void write_log(quill::LogRecord const& record) override
    {
      messages.emplace_back(record.log_message);

      if (record.named_args)
      {
        for (auto const& named_arg : *record.named_args)
        {
          named_args.emplace_back(named_arg.first + "=" + named_arg.second);
        }
      }
    }
    void flush_sink(quill::SinkFlushReason) override {}
    std::vector<std::string> messages;
    std::vector<std::string> named_args;
  };

  size_t errors{0};
  quill::BackendOptions options;
  options.log_timestamp_ordering_grace_period = std::chrono::microseconds{0};
  // A buffer of four events, so that the named arguments below reuse the ones of earlier events
  options.transit_event_buffer_initial_capacity = 4;
  options.error_notifier = [&errors](std::string const&) { ++errors; };
  auto* worker = quill::Backend::acquire_manual_backend_worker();
  worker->init(options);

  auto sink = quill::Frontend::create_or_get_sink<CapturingSink>("decode_allocation_failure_sink");
  auto* logger = quill::Frontend::create_or_get_logger("decode_allocation_failure_logger", sink);

  // Discover this thread's context and grow the backend's scratch buffers
  LOG_INFO(logger, "warmup {} {}", 1, 2);
  worker->poll();
  sink->messages.clear();

  // poll() discovering a new frontend thread must report an allocation failure instead of
  // letting it escape to the caller
  std::thread producer([logger]() { LOG_INFO(logger, "from another thread"); });
  producer.join();
  fail_next_allocation = true;
  worker->poll();
  CHECK_EQ(errors, 1u);
  worker->poll();
  REQUIRE_EQ(sink->messages.size(), 1u);
  CHECK_EQ(sink->messages[0], "from another thread");
  sink->messages.clear();

  // The vector argument allocates while decoding; after the failure the record is decoded again
  // and must not be formatted with the int left over from the failed attempt
  std::vector<int> const values{7, 8, 9};
  LOG_INFO(logger, "id={} values={}", 42, values);
  fail_next_allocation = true;
  worker->poll_one(); // reported as the same bad_alloc, so the notifier suppresses the repeat
  worker->poll();

  REQUIRE_EQ(sink->messages.size(), 1u);
  CHECK_EQ(sink->messages[0], "id=42 values=[7, 8, 9]");
  sink->messages.clear();

  // Give each transit event of the buffer its named arguments, with values that need no allocation
  auto const log_order = [logger](int id, std::string const& user)
  { LOG_INFO(logger, "order {id} from {user}", id, user); };

  for (int id = 0; id < 4; ++id)
  {
    log_order(id, "short");
    worker->poll();
  }

  sink->messages.clear();
  sink->named_args.clear();

  // The value of `user` now needs an allocation. A failure leaves that value empty and the message
  // is formatted from the arguments. The record is not decoded again: a value that never gets its
  // memory would hold up the records behind it
  std::string const user(40, 'u');
  log_order(51, user);
  log_order(52, "short");
  fail_next_allocation = true;
  worker->poll();

  REQUIRE_FALSE(fail_next_allocation);
  REQUIRE_EQ(sink->messages.size(), 2u);
  CHECK_EQ(sink->messages[0], "order 51 from " + user);
  CHECK_EQ(sink->messages[1], "order 52 from short");
  REQUIRE_EQ(sink->named_args.size(), 4u);
  CHECK_EQ(sink->named_args[0], "id=51");
  CHECK_EQ(sink->named_args[1], "user=");
  CHECK_EQ(sink->named_args[2], "id=52");
  CHECK_EQ(sink->named_args[3], "user=short");
  sink->messages.clear();
  sink->named_args.clear();

  // A value with non-printable characters is sanitized into a longer string. That is the first
  // allocation of the poll below, as the short message and value need none before they are
  // sanitized. A failure leaves the value empty, and the record is not decoded again
  auto const log_tag = [logger](std::string const& tag) { LOG_INFO(logger, "{tag}", tag); };
  log_tag("warmup");
  worker->poll();
  sink->messages.clear();
  sink->named_args.clear();

  std::string const control_characters(10, '\x01');
  log_tag(control_characters);
  log_order(53, "short");
  fail_next_allocation = true;
  worker->poll();

  REQUIRE_FALSE(fail_next_allocation);
  REQUIRE_EQ(sink->messages.size(), 2u);
  CHECK_EQ(sink->messages[0], R"(\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01)");
  CHECK_EQ(sink->messages[1], "order 53 from short");
  REQUIRE_EQ(sink->named_args.size(), 3u);
  CHECK_EQ(sink->named_args[0], "tag=");
  CHECK_EQ(sink->named_args[1], "id=53");
  CHECK_EQ(sink->named_args[2], "user=short");

  worker->shutdown();
  quill::Frontend::remove_logger(logger);
#endif
}
