#include "DocTestExtensions.h"
#include "doctest/doctest.h"

#include "quill/backend/TransitEvent.h"
#include "quill/core/LogLevel.h"
#include "quill/core/MacroMetadata.h"

#include <cstddef>
#include <new>
#include <string>

// ThreadSanitizer supplies its own global new/delete definitions, which cannot be replaced.
#if !defined(QUILL_NO_EXCEPTIONS) && !QUILL_HAS_FEATURE(thread_sanitizer) && !defined(__SANITIZE_THREAD__)
namespace
{
thread_local bool fail_next_allocation{false};
} // namespace

// This executable replaces global operator new to simulate a single allocation failure
// without exhausting memory. Other test executables use their normal allocator.
void* operator new(std::size_t size)
{
  if (fail_next_allocation)
  {
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

TEST_SUITE_BEGIN("TransitEvent");

using namespace quill;
using namespace quill::detail;

/***/
TEST_CASE("transit_event_copy_points_to_its_own_runtime_metadata")
{
  TransitEvent runtime_event;
  runtime_event.ensure_runtime_metadata().set("app.cpp", 10, "main", nullptr, "runtime {}", LogLevel::Info);
  runtime_event.macro_metadata = &runtime_event.extra_data->runtime_metadata->macro_metadata;

  TransitEvent runtime_copy;
  runtime_event.copy_to(runtime_copy);
  REQUIRE_EQ(runtime_copy.macro_metadata, &runtime_copy.extra_data->runtime_metadata->macro_metadata);
  REQUIRE_STREQ(runtime_copy.macro_metadata->message_format(), "runtime {}");

  // The event of a log statement keeps the metadata of the statement, also in a slot that has
  // runtime metadata from a previous event
  static constexpr MacroMetadata statement_metadata{
    "app.cpp:20", "main", "statement {}", nullptr, LogLevel::Info, MacroMetadata::Event::Log};
  runtime_event.macro_metadata = &statement_metadata;

  TransitEvent statement_copy;
  runtime_event.copy_to(statement_copy);
  REQUIRE_EQ(statement_copy.macro_metadata, &statement_metadata);
}

/***/
TEST_CASE("runtime_metadata_is_set_again_after_an_allocation_failure")
{
#if defined(QUILL_NO_EXCEPTIONS) || QUILL_HAS_FEATURE(thread_sanitizer) || defined(__SANITIZE_THREAD__)
  return;
#elif defined(_ITERATOR_DEBUG_LEVEL) && (_ITERATOR_DEBUG_LEVEL > 0)
  // The MSVC debug STL also allocates container proxies, so another allocation would fail
  return;
#else
  // The source location fits the small string buffer of every standard library with the first
  // line number and needs an allocation with the second
  static constexpr char const* file = "abcdefgh.cpp";
  static constexpr uint32_t first_line = 9;
  static constexpr uint32_t second_line = 4000000000u;

  TransitEvent::RuntimeMetadata runtime_metadata;
  runtime_metadata.set(file, first_line, "main", nullptr, "first {}", LogLevel::Info);
  REQUIRE_FALSE(runtime_metadata.macro_metadata.has_named_args());

  // The format string and the line number are replaced before the source location fails to grow
  bool failed{false};
  fail_next_allocation = true;

  try
  {
    runtime_metadata.set(file, second_line, "main", nullptr, "second {name}", LogLevel::Info);
  }
  catch (std::bad_alloc const&)
  {
    failed = true;
  }

  fail_next_allocation = false;
  REQUIRE(failed);

  // The backend decodes the record again. The members that the failed call replaced equal the
  // arguments, which used to keep the macro_metadata of the first call
  runtime_metadata.set(file, second_line, "main", nullptr, "second {name}", LogLevel::Info);
  REQUIRE_EQ(std::string{runtime_metadata.macro_metadata.message_format()}, "second {name}");
  REQUIRE(runtime_metadata.macro_metadata.has_named_args());
  REQUIRE_EQ(std::string{runtime_metadata.macro_metadata.line()}, "4000000000");
#endif
}

TEST_SUITE_END();
