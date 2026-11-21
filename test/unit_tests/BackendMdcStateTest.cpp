#include "doctest/doctest.h"

#include "quill/backend/BackendMdcState.h"

#include <cstddef>
#include <new>

// ThreadSanitizer supplies its own global new/delete definitions, which cannot be replaced.
#if !defined(QUILL_NO_EXCEPTIONS) && !QUILL_HAS_FEATURE(thread_sanitizer) && !defined(__SANITIZE_THREAD__)
namespace
{
thread_local bool fail_next_allocation{false};
}

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

TEST_SUITE_BEGIN("BackendMdcState");

using namespace quill;
using namespace quill::detail;

TEST_CASE("backend_mdc_state_empty_rebuild_keeps_empty_output")
{
  BackendMdcState mdc_state{"[{}={};]"};

  REQUIRE(mdc_state.empty());
  REQUIRE(mdc_state.formatted_mdc().empty());

  mdc_state.rebuild_formatted_mdc();

  REQUIRE(mdc_state.empty());
  REQUIRE(mdc_state.formatted_mdc().empty());
}

TEST_CASE("backend_mdc_state_formats_fields_in_key_order")
{
  BackendMdcState mdc_state{"[{}={};]"};

  mdc_state.set("b", "2");
  mdc_state.set("a", "1");
  mdc_state.rebuild_formatted_mdc();

  REQUIRE_FALSE(mdc_state.empty());
  REQUIRE_EQ(std::string{mdc_state.formatted_mdc()}, "[a=1;b=2]");
}

TEST_CASE("backend_mdc_state_overwrite_and_erase_update_output")
{
  BackendMdcState mdc_state{"[{}={};]"};

  mdc_state.set("a", "1");
  mdc_state.set("a", "2");
  mdc_state.set("b", "3");
  mdc_state.rebuild_formatted_mdc();
  REQUIRE_EQ(std::string{mdc_state.formatted_mdc()}, "[a=2;b=3]");

  mdc_state.erase("a");
  mdc_state.rebuild_formatted_mdc();
  REQUIRE_EQ(std::string{mdc_state.formatted_mdc()}, "[b=3]");

  mdc_state.erase("missing");
  mdc_state.rebuild_formatted_mdc();
  REQUIRE_EQ(std::string{mdc_state.formatted_mdc()}, "[b=3]");

  mdc_state.erase("b");
  mdc_state.rebuild_formatted_mdc();
  REQUIRE(mdc_state.empty());
  REQUIRE(mdc_state.formatted_mdc().empty());
}

TEST_CASE("backend_mdc_state_multi_char_separator_and_suffix")
{
  // Exercise the field_sep / suffix extraction: separator is ", " (two chars) and suffix is "]".
  // This mirrors the documented default BackendOptions::mdc_format_pattern " [{}: {}, ]".
  BackendMdcState mdc_state{" [{}: {}, ]"};

  mdc_state.set("a", "1");
  mdc_state.set("b", "2");
  mdc_state.set("c", "3");
  mdc_state.rebuild_formatted_mdc();

  REQUIRE_EQ(std::string{mdc_state.formatted_mdc()}, " [a: 1, b: 2, c: 3]");
}

TEST_CASE("backend_mdc_state_single_field_no_trailing_separator")
{
  // A single field must not emit the field separator at all.
  BackendMdcState mdc_state{" [{}: {}, ]"};

  mdc_state.set("only", "value");
  mdc_state.rebuild_formatted_mdc();

  REQUIRE_EQ(std::string{mdc_state.formatted_mdc()}, " [only: value]");
}

TEST_CASE("backend_mdc_state_longer_field_separator")
{
  // Prove field_sep extraction is not off-by-one when the separator is longer than 1 char.
  // Here separator is " | " (3 chars), suffix is "}".
  BackendMdcState mdc_state{"{{}={} | }"};

  mdc_state.set("a", "1");
  mdc_state.set("b", "2");
  mdc_state.rebuild_formatted_mdc();

  REQUIRE_EQ(std::string{mdc_state.formatted_mdc()}, "{a=1 | b=2}");
}

TEST_CASE("backend_mdc_state_invalid_pattern_throws")
{
#if !defined(QUILL_NO_EXCEPTIONS)
  REQUIRE_THROWS_AS(BackendMdcState{"invalid"}, QuillError);
  REQUIRE_THROWS_AS(BackendMdcState{"{}"}, QuillError);
  REQUIRE_THROWS_AS(BackendMdcState{"{} {}"}, QuillError);
  REQUIRE_THROWS_AS(BackendMdcState{"{} {} {}!"}, QuillError);
  REQUIRE_THROWS_AS(BackendMdcState{"prefix {} middle {}"}, QuillError);
#endif
}

TEST_CASE("backend_mdc_state_preserves_last_rendering_when_rebuild_fails")
{
#if !defined(QUILL_NO_EXCEPTIONS) && !QUILL_HAS_FEATURE(thread_sanitizer) && !defined(__SANITIZE_THREAD__)
  BackendMdcState state{"[{}={};]"};
  state.set("a", "before");
  state.rebuild_formatted_mdc();
  REQUIRE_EQ(std::string{state.formatted_mdc()}, "[a=before]");

  // Too long for the small string buffer, so rendering it has to allocate
  std::string const long_value(256, 'x');
  state.set("b", long_value);

  bool failed{false};
  fail_next_allocation = true;

  try
  {
    state.rebuild_formatted_mdc();
  }
  catch (std::bad_alloc const&)
  {
    failed = true;
  }

  fail_next_allocation = false;

  // The rendering used to be rebuilt in place, which left it truncated after the failure
  REQUIRE(failed);
  REQUIRE_EQ(std::string{state.formatted_mdc()}, "[a=before]");

  state.rebuild_formatted_mdc();
  REQUIRE_EQ(std::string{state.formatted_mdc()}, "[a=before;b=" + long_value + "]");
#endif
}

TEST_SUITE_END();
