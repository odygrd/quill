#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/DeferredFormatCodec.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/Sink.h"
#include "quill/std/Vector.h"

#include <cstddef>
#include <memory>
#include <new>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// ThreadSanitizer supplies its own global new/delete definitions, which cannot be replaced.
// The test also skips the MSVC debug STL because allocating container proxies can terminate.
#if !defined(QUILL_NO_EXCEPTIONS) && !QUILL_HAS_FEATURE(thread_sanitizer) &&                       \
  !defined(__SANITIZE_THREAD__) && (!defined(_ITERATOR_DEBUG_LEVEL) || (_ITERATOR_DEBUG_LEVEL == 0))
namespace
{
// Only the calling thread's next allocation fails; producer allocations are unaffected.
thread_local bool fail_next_allocation{false};

// Set to fail the first allocation that follows the formatting of a payload
bool fail_allocation_after_payload_format{false};

// Set to fail the next allocation of a FailingAllocator
bool fail_next_failing_allocator_allocation{false};

/** Fails an allocation on request, to make the decoder of a std::vector throw */
template <typename T>
struct FailingAllocator
{
  using value_type = T;

  FailingAllocator() = default;

  template <typename U>
  FailingAllocator(FailingAllocator<U> const&) noexcept
  {
  }

  T* allocate(std::size_t count)
  {
    if (fail_next_failing_allocator_allocation)
    {
      fail_next_failing_allocator_allocation = false;
      throw std::bad_alloc{};
    }

    return std::allocator<T>{}.allocate(count);
  }

  void deallocate(T* memory, std::size_t count) noexcept
  {
    std::allocator<T>{}.deallocate(memory, count);
  }

  template <typename U>
  bool operator==(FailingAllocator<U> const&) const noexcept
  {
    return true;
  }

  template <typename U>
  bool operator!=(FailingAllocator<U> const&) const noexcept
  {
    return false;
  }
};

/** Owns heap memory and can only be copied */
struct CopyOnlyPayload
{
  explicit CopyOnlyPayload(std::string value) : text(std::move(value)) { ++live_count; }
  CopyOnlyPayload(CopyOnlyPayload const& other) : text(other.text) { ++live_count; }
  CopyOnlyPayload(CopyOnlyPayload&&) = delete;
  CopyOnlyPayload& operator=(CopyOnlyPayload const&) = delete;
  CopyOnlyPayload& operator=(CopyOnlyPayload&&) = delete;
  ~CopyOnlyPayload() { --live_count; }

  std::string text;
  static int live_count;
};

int CopyOnlyPayload::live_count{0};

/** Owns heap memory and can be moved: a decoder that moved it out of the queue would leave it empty there */
struct MovablePayload
{
  explicit MovablePayload(std::string value) : text(std::move(value)) { ++live_count; }
  MovablePayload(MovablePayload&& other) noexcept : text(std::move(other.text)) { ++live_count; }
  MovablePayload(MovablePayload const&) = delete;
  MovablePayload& operator=(MovablePayload const&) = delete;
  MovablePayload& operator=(MovablePayload&&) = delete;
  ~MovablePayload() { --live_count; }

  std::string text;
  static int live_count;
};

int MovablePayload::live_count{0};

/** Owns heap memory and has no formatter: it is formatted through its conversion to std::string */
struct TextPayload
{
  explicit TextPayload(std::string value) : text(std::move(value)) { ++live_count; }
  TextPayload(TextPayload&& other) noexcept : text(std::move(other.text)) { ++live_count; }
  TextPayload(TextPayload const&) = delete;
  TextPayload& operator=(TextPayload const&) = delete;
  TextPayload& operator=(TextPayload&&) = delete;
  ~TextPayload() { --live_count; }

  operator std::string() const { return text; }

  std::string text;
  static int live_count;
};

int TextPayload::live_count{0};

/** Owns heap memory and has no formatter: it is formatted as the number that format_as() returns */
struct NumberPayload
{
  explicit NumberPayload(int value) : numbers(64, value) { ++live_count; }
  NumberPayload(NumberPayload&& other) noexcept : numbers(std::move(other.numbers))
  {
    ++live_count;
  }
  NumberPayload(NumberPayload const&) = delete;
  NumberPayload& operator=(NumberPayload const&) = delete;
  NumberPayload& operator=(NumberPayload&&) = delete;
  ~NumberPayload() { --live_count; }

  std::vector<int> numbers;
  static int live_count;
};

int NumberPayload::live_count{0};

int format_as(NumberPayload const& payload) { return payload.numbers.front(); }

/** Formats the text of a payload, and fails the allocation that follows when the test asks for it */
template <typename TPayload>
auto format_payload(TPayload const& payload, fmtquill::format_context& ctx)
{
  auto out = fmtquill::format_to(ctx.out(), "{}", payload.text);

  if (fail_allocation_after_payload_format)
  {
    fail_allocation_after_payload_format = false;
    fail_next_allocation = true;
  }

  return out;
}
} // namespace

template <>
struct fmtquill::formatter<CopyOnlyPayload>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(CopyOnlyPayload const& payload, format_context& ctx) const
  {
    return format_payload(payload, ctx);
  }
};

template <>
struct quill::Codec<CopyOnlyPayload> : quill::DeferredFormatCodec<CopyOnlyPayload>
{
};

template <>
struct fmtquill::formatter<MovablePayload>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(MovablePayload const& payload, format_context& ctx) const
  {
    return format_payload(payload, ctx);
  }
};

template <>
struct quill::Codec<MovablePayload> : quill::DeferredFormatCodec<MovablePayload>
{
};

template <>
struct quill::Codec<TextPayload> : quill::DeferredFormatCodec<TextPayload>
{
};

template <>
struct quill::Codec<NumberPayload> : quill::DeferredFormatCodec<NumberPayload>
{
};

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
      mdcs.emplace_back(record.mdc);

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
    std::vector<std::string> mdcs;
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
  sink->messages.clear();

  // A statement that is new to the backend needs memory for the template of its named arguments.
  // When there is none, the record is written with the error in place of its message, and
  // without the named arguments that its transit event has kept from an earlier record
  auto const log_thing = [logger](std::string const& thing)
  { LOG_INFO(logger, "one {thing}", thing); };
  sink->named_args.clear();
  log_thing("x");
  fail_next_allocation = true;
  worker->poll();

  REQUIRE_FALSE(fail_next_allocation);
  REQUIRE_EQ(sink->messages.size(), 1u);
  CHECK_EQ(sink->messages[0].rfind("[Could not format log statement. message: \"one {thing}\"", 0), 0u);
  CHECK(sink->named_args.empty());
  sink->messages.clear();

  // A CopyOnlyPayload is formatted where it is in the queue. The MDC of the record is copied
  // after the message is formatted; when that allocation fails the record is not decoded a second
  // time, and the payload in the queue is destroyed once
  std::string const request_id(64, 'r');
  logger->set_mdc("request", request_id);
  worker->poll();

  {
    CopyOnlyPayload const payload{std::string(64, 'p')};
    LOG_INFO(logger, "payload {}", payload);
    LOG_INFO(logger, "after payload");
    fail_allocation_after_payload_format = true;
    worker->poll();
  }

  REQUIRE_FALSE(fail_allocation_after_payload_format);
  REQUIRE_FALSE(fail_next_allocation);
  REQUIRE_EQ(sink->messages.size(), 2u);
  CHECK_EQ(sink->messages[0].rfind("[Could not format log statement. message: \"payload {}\"", 0), 0u);
  CHECK_EQ(sink->messages[1], "after payload");
  CHECK_EQ(CopyOnlyPayload::live_count, 0);
  sink->messages.clear();

  // The decoder of the vector throws after the decoder of the CopyOnlyPayload. The record stays
  // in the queue and is decoded again, so the payload must still be alive there
  {
    CopyOnlyPayload const payload{std::string(64, 'q')};
    std::vector<int, FailingAllocator<int>> const numbers{1, 2, 3};
    LOG_INFO(logger, "payload {} numbers {}", payload, numbers);
    fail_next_failing_allocator_allocation = true;
    worker->poll_one();
    REQUIRE_FALSE(fail_next_failing_allocator_allocation);
    REQUIRE(sink->messages.empty());
    worker->poll();
  }

  REQUIRE_EQ(sink->messages.size(), 1u);
  CHECK_EQ(sink->messages[0], "payload " + std::string(64, 'q') + " numbers [1, 2, 3]");
  CHECK_EQ(CopyOnlyPayload::live_count, 0);
  sink->messages.clear();

  // A MovablePayload is not moved out of the queue either: the record that is decoded again is
  // written with the text that was logged
  {
    std::vector<int, FailingAllocator<int>> const numbers{1, 2, 3};
    LOG_INFO(logger, "payload {} numbers {}", MovablePayload{std::string(64, 'm')}, numbers);
    fail_next_failing_allocator_allocation = true;
    worker->poll_one();
    REQUIRE_FALSE(fail_next_failing_allocator_allocation);
    REQUIRE(sink->messages.empty());
    worker->poll();
  }

  REQUIRE_EQ(sink->messages.size(), 1u);
  CHECK_EQ(sink->messages[0], "payload " + std::string(64, 'm') + " numbers [1, 2, 3]");
  CHECK_EQ(MovablePayload::live_count, 0);
  sink->messages.clear();

  // An object without a formatter of its own stays alive in the queue as well: one that is
  // formatted through its conversion to std::string, and one that format_as() maps to a number
  {
    std::vector<int, FailingAllocator<int>> const numbers{1, 2, 3};
    LOG_INFO(logger, "text {} number {} numbers {}", TextPayload{std::string(64, 't')},
             NumberPayload{42}, numbers);
    fail_next_failing_allocator_allocation = true;
    worker->poll_one();
    REQUIRE_FALSE(fail_next_failing_allocator_allocation);
    REQUIRE(sink->messages.empty());
    CHECK_EQ(TextPayload::live_count, 1);
    CHECK_EQ(NumberPayload::live_count, 1);
    worker->poll();
  }

  REQUIRE_EQ(sink->messages.size(), 1u);
  CHECK_EQ(sink->messages[0], "text " + std::string(64, 't') + " number 42 numbers [1, 2, 3]");
  CHECK_EQ(TextPayload::live_count, 0);
  CHECK_EQ(NumberPayload::live_count, 0);
  sink->messages.clear();
  sink->mdcs.clear();

  // The value of an MDC field is formatted from the queue as well. Keeping the field allocates
  // after the arguments were decoded; when that fails the request stays in the queue and is
  // decoded again, and the field must get the value that was set
  logger->set_mdc("payload", MovablePayload{"mdc value"});
  fail_allocation_after_payload_format = true;
  worker->poll_one();
  REQUIRE_FALSE(fail_allocation_after_payload_format);
  REQUIRE_FALSE(fail_next_allocation);
  worker->poll();

  LOG_INFO(logger, "with mdc");
  worker->poll();

  REQUIRE_EQ(sink->mdcs.size(), 1u);
  CHECK_NE(sink->mdcs[0].find("payload: mdc value"), std::string::npos);
  CHECK_EQ(MovablePayload::live_count, 0);

  worker->shutdown();
  quill::Frontend::remove_logger(logger);
#endif
}
