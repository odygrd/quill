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

  // The runtime metadata that the event does not use is not copied: it may refer to the strings
  // of a previous record that no longer exist
  REQUIRE_FALSE(statement_copy.extra_data->runtime_metadata);
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

  // The format string is replaced before the source location fails to grow
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

  // The backend decodes the record again with the same arguments. The members that the failed
  // call replaced equal them, and the metadata must still be built for them
  runtime_metadata.set(file, second_line, "main", nullptr, "second {name}", LogLevel::Info);
  REQUIRE_EQ(std::string{runtime_metadata.macro_metadata.message_format()}, "second {name}");
  REQUIRE(runtime_metadata.macro_metadata.has_named_args());
  REQUIRE_EQ(std::string{runtime_metadata.macro_metadata.line()}, "4000000000");
#endif
}

/***/
TEST_CASE("runtime_metadata_of_a_literal_format_string_refers_to_its_strings")
{
  static constexpr char const* fmt = "Request {} done at {stage}";
  static constexpr char const* file = "/src/app/parse.cpp";
  static constexpr char const* function = "on_read";

  MacroMetadata const facts{file,    function,       fmt,
                            nullptr, LogLevel::Info, MacroMetadata::Event::Log};
  StaticRuntimeMetadata const static_metadata{
    fmt, file, function, StaticRuntimeMetadata::pack(42, 26, 18, 9, facts.format_flags())};

  // The slot of the event had the metadata of a call with tags before
  TransitEvent::RuntimeMetadata runtime_metadata;
  runtime_metadata.set("old.cpp", 7, "old", "#old ", "old {}", LogLevel::Debug);
  runtime_metadata.set_static(static_metadata, LogLevel::Warning);

  MacroMetadata const& macro_metadata = runtime_metadata.macro_metadata;
  REQUIRE(macro_metadata.message_format() == fmt);
  REQUIRE(macro_metadata.caller_function() == function);
  REQUIRE(macro_metadata.tags() == nullptr);
  REQUIRE_EQ(macro_metadata.log_level(), LogLevel::Warning);
  REQUIRE_EQ(macro_metadata.event(), MacroMetadata::Event::Log);

  // The same metadata as for a copied format string
  TransitEvent::RuntimeMetadata const copied{file, 42, function, nullptr, fmt, LogLevel::Warning};
  REQUIRE_EQ(std::string{macro_metadata.source_location()}, "/src/app/parse.cpp:42");
  REQUIRE_EQ(std::string{macro_metadata.source_location()},
             std::string{copied.macro_metadata.source_location()});
  REQUIRE_EQ(macro_metadata.full_path(), copied.macro_metadata.full_path());
  REQUIRE_EQ(macro_metadata.file_name(), copied.macro_metadata.file_name());
  REQUIRE_EQ(std::string{macro_metadata.short_source_location()},
             std::string{copied.macro_metadata.short_source_location()});
  REQUIRE_EQ(std::string{macro_metadata.line()}, "42");
  REQUIRE_EQ(macro_metadata.message_format_size(), copied.macro_metadata.message_format_size());
  REQUIRE_EQ(macro_metadata.has_named_args(), copied.macro_metadata.has_named_args());
  REQUIRE(macro_metadata.has_named_args());
  REQUIRE_EQ(macro_metadata.has_only_empty_fields(), copied.macro_metadata.has_only_empty_fields());

  // A copy, e.g. of a backtrace record, owns its strings: it can outlive the literals of the call
  TransitEvent::RuntimeMetadata const copy{runtime_metadata};
  REQUIRE(copy.macro_metadata.message_format() == copy.fmt.data());
  REQUIRE_EQ(copy.fmt, std::string{fmt});
  REQUIRE_EQ(copy.macro_metadata.message_format_size(), 26u);
  REQUIRE(copy.macro_metadata.caller_function() == copy.function_name.data());
  REQUIRE_EQ(copy.function_name, std::string{function});
  REQUIRE(copy.macro_metadata.tags() == nullptr);
  REQUIRE_EQ(copy.macro_metadata.log_level(), LogLevel::Warning);
  REQUIRE(copy.macro_metadata.has_named_args());
  REQUIRE(copy.macro_metadata.source_location() != macro_metadata.source_location());
  REQUIRE_EQ(std::string{copy.macro_metadata.source_location()}, "/src/app/parse.cpp:42");
  REQUIRE_EQ(copy.macro_metadata.file_name(), std::string_view{"parse.cpp"});

  // The metadata of a copied format string is set again over it
  runtime_metadata.set(file, 43, function, "#tag ", "copied {}", LogLevel::Info);
  TransitEvent::RuntimeMetadata const owned_copy{runtime_metadata};
  REQUIRE_EQ(std::string{owned_copy.macro_metadata.message_format()}, "copied {}");
  REQUIRE(owned_copy.macro_metadata.message_format() == owned_copy.fmt.data());
  REQUIRE_EQ(std::string{owned_copy.macro_metadata.tags()}, "#tag ");
}

/***/
TEST_CASE("runtime_metadata_of_a_literal_format_string_without_file_info")
{
  StaticRuntimeMetadata const static_metadata{"no file info", "", "",
                                              StaticRuntimeMetadata::pack(0, 12, 0, 0, 2u)};

  TransitEvent::RuntimeMetadata runtime_metadata;
  runtime_metadata.set_static(static_metadata, LogLevel::Info);

  // As for a copied format string, the source location is empty
  TransitEvent::RuntimeMetadata const copied{"", 0, "", nullptr, "no file info", LogLevel::Info};
  REQUIRE_EQ(std::string{runtime_metadata.macro_metadata.source_location()}, "");
  REQUIRE_EQ(std::string{copied.macro_metadata.source_location()}, "");
  REQUIRE_EQ(runtime_metadata.macro_metadata.full_path(), copied.macro_metadata.full_path());
  REQUIRE_EQ(std::string{runtime_metadata.macro_metadata.line()}, std::string{copied.macro_metadata.line()});
  REQUIRE_EQ(runtime_metadata.macro_metadata.message_format_size(), 12u);
}

TEST_SUITE_END();
