#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include "quill/DeferredFormatCodec.h"
#include "quill/bundled/fmt/format.h"

#include <atomic>
#include <cstdio>
#include <string>

using namespace quill;

/***/
class MoveOnlyTypeWithCounter
{
public:
  static inline std::atomic<uint32_t> move_ctor_count{0};
  static inline std::atomic<uint32_t> dtor_count{0};

  MoveOnlyTypeWithCounter() = default;
  ~MoveOnlyTypeWithCounter() { ++dtor_count; }

  explicit MoveOnlyTypeWithCounter(std::string value) : value(std::move(value)) {}

  MoveOnlyTypeWithCounter(MoveOnlyTypeWithCounter&& other) noexcept : value(std::move(other.value))
  {
    ++move_ctor_count;
  }

  MoveOnlyTypeWithCounter& operator=(MoveOnlyTypeWithCounter&&) = default;

  // Delete copy operations to make it move-only
  MoveOnlyTypeWithCounter(MoveOnlyTypeWithCounter const&) = delete;
  MoveOnlyTypeWithCounter& operator=(MoveOnlyTypeWithCounter const&) = delete;

  std::string value;
};

/***/
template <>
struct fmtquill::formatter<MoveOnlyTypeWithCounter>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::MoveOnlyTypeWithCounter const& obj, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "MoveOnly({})", obj.value);
  }
};

/***/
template <>
struct quill::Codec<MoveOnlyTypeWithCounter> : quill::DeferredFormatCodec<MoveOnlyTypeWithCounter>
{
};

/***/
class CopyOnlyTypeWithCounter
{
public:
  static inline std::atomic<uint32_t> copy_ctor_count{0};
  static inline std::atomic<uint32_t> dtor_count{0};

  CopyOnlyTypeWithCounter() = default;
  ~CopyOnlyTypeWithCounter() { ++dtor_count; }

  explicit CopyOnlyTypeWithCounter(std::string value) : value(value) {}

  CopyOnlyTypeWithCounter(CopyOnlyTypeWithCounter const& other) : value(other.value)
  {
    ++copy_ctor_count;
  }

  CopyOnlyTypeWithCounter& operator=(CopyOnlyTypeWithCounter const&) = default;

  // Delete move operations to make it copy-only
  CopyOnlyTypeWithCounter(CopyOnlyTypeWithCounter&&) = delete;
  CopyOnlyTypeWithCounter& operator=(CopyOnlyTypeWithCounter&&) = delete;

  std::string value;
};

/***/
template <>
struct fmtquill::formatter<CopyOnlyTypeWithCounter>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::CopyOnlyTypeWithCounter const& obj, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "CopyOnly({})", obj.value);
  }
};

/***/
template <>
struct quill::Codec<CopyOnlyTypeWithCounter> : quill::DeferredFormatCodec<CopyOnlyTypeWithCounter>
{
};

/***/
class MoveAndCopyTypeWithCounter
{
public:
  static inline std::atomic<uint32_t> move_ctor_count{0};
  static inline std::atomic<uint32_t> copy_ctor_count{0};
  static inline std::atomic<uint32_t> dtor_count{0};

  MoveAndCopyTypeWithCounter() = default;
  ~MoveAndCopyTypeWithCounter() { ++dtor_count; }

  explicit MoveAndCopyTypeWithCounter(std::string value) : value(std::move(value)) {}

  MoveAndCopyTypeWithCounter(MoveAndCopyTypeWithCounter&& other) noexcept
    : value(std::move(other.value))
  {
    ++move_ctor_count;
  }

  MoveAndCopyTypeWithCounter& operator=(MoveAndCopyTypeWithCounter&&) = default;

  MoveAndCopyTypeWithCounter(MoveAndCopyTypeWithCounter const& other) : value(other.value)
  {
    ++copy_ctor_count;
  }

  MoveAndCopyTypeWithCounter& operator=(MoveAndCopyTypeWithCounter const&) = default;

  std::string value;
};

/***/
template <>
struct fmtquill::formatter<MoveAndCopyTypeWithCounter>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::MoveAndCopyTypeWithCounter const& obj, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "MoveAndCopy({})", obj.value);
  }
};

/***/
template <>
struct quill::Codec<MoveAndCopyTypeWithCounter> : quill::DeferredFormatCodec<MoveAndCopyTypeWithCounter>
{
};

/***/
TEST_CASE("custom_type_defined_type_deferred_format_logging_move_and_copy_semantics")
{
  static constexpr char const* filename =
    "custom_type_defined_type_deferred_format_logging_move_and_copy_semantics.log";
  static std::string const logger_name = "logger";

  Frontend::preallocate();

  // Create a file sink
  auto file_sink = Frontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));

  // Reset all counters
  MoveOnlyTypeWithCounter::move_ctor_count = 0;
  CopyOnlyTypeWithCounter::copy_ctor_count = 0;
  MoveAndCopyTypeWithCounter::move_ctor_count = 0;
  MoveAndCopyTypeWithCounter::copy_ctor_count = 0;

  // Test 1: MoveOnlyType with rvalue (std::move)
  {
    MoveOnlyTypeWithCounter move_obj{"MoveOnlyValue"};
    LOG_INFO(logger, "Test: {}", std::move(move_obj));
  }

  // Test 2: CopyOnlyType with lvalue (should copy)
  {
    CopyOnlyTypeWithCounter copy_obj{"CopyOnlyValue"};
    LOG_INFO(logger, "Test: {}", copy_obj);
  }

  // Test 3: MoveAndCopyType with lvalue (should copy to preserve object)
  {
    MoveAndCopyTypeWithCounter both_lvalue{"BothLvalue"};
    LOG_INFO(logger, "Test: {}", both_lvalue);
    // Verify object is still usable (was copied, not moved)
    REQUIRE_EQ(both_lvalue.value, "BothLvalue");
  }

  // Test 4: MoveAndCopyType with rvalue (should move)
  {
    MoveAndCopyTypeWithCounter both_rvalue{"BothRvalue"};
    LOG_INFO(logger, "Test: {}", std::move(both_rvalue));
  }

  // Encode Phase Counters
  REQUIRE_EQ(MoveOnlyTypeWithCounter::move_ctor_count.load(), 1);
  REQUIRE_EQ(CopyOnlyTypeWithCounter::copy_ctor_count.load(), 1);
  REQUIRE_EQ(MoveAndCopyTypeWithCounter::copy_ctor_count.load(), 1);
  REQUIRE_EQ(MoveAndCopyTypeWithCounter::move_ctor_count.load(), 1);

  // Reset all counters
  MoveOnlyTypeWithCounter::move_ctor_count = 0;
  CopyOnlyTypeWithCounter::copy_ctor_count = 0;
  MoveAndCopyTypeWithCounter::move_ctor_count = 0;
  MoveAndCopyTypeWithCounter::copy_ctor_count = 0;

  // Start the logging backend thread
  Backend::start();

  logger->flush_log();

  // Decode Phase Counters
  // Note: Exact counts can vary due to compiler optimizations (RVO/NRVO), especially across
  // different compilers (GCC, Clang, MSVC) and build modes (Debug/Release). We verify that
  // moves/copies happen but allow for optimization to reduce the count.
  REQUIRE_GE(MoveOnlyTypeWithCounter::move_ctor_count.load(), 2);
  REQUIRE_LE(MoveOnlyTypeWithCounter::move_ctor_count.load(), 3);
  REQUIRE_GE(CopyOnlyTypeWithCounter::copy_ctor_count.load(), 2);
  REQUIRE_LE(CopyOnlyTypeWithCounter::copy_ctor_count.load(), 3);
  REQUIRE_GE(MoveAndCopyTypeWithCounter::move_ctor_count.load(), 4);
  REQUIRE_LE(MoveAndCopyTypeWithCounter::move_ctor_count.load(), 6);

  // Test destruction
  MoveOnlyTypeWithCounter::move_ctor_count = 0;
  MoveOnlyTypeWithCounter::dtor_count = 0;
  {
    MoveOnlyTypeWithCounter move_obj{"MoveOnlyValue"};
    LOG_INFO(logger, "Test: {}", std::move(move_obj));
  }
  logger->flush_log();

  REQUIRE_GE(MoveOnlyTypeWithCounter::move_ctor_count.load(), 3);
  REQUIRE_LE(MoveOnlyTypeWithCounter::move_ctor_count.load(), 4);
  REQUIRE_GE(MoveOnlyTypeWithCounter::dtor_count.load(), 4);
  REQUIRE_LE(MoveOnlyTypeWithCounter::dtor_count.load(), 5);

  // Test destruction
  CopyOnlyTypeWithCounter::copy_ctor_count = 0;
  CopyOnlyTypeWithCounter::dtor_count = 0;
  {
    CopyOnlyTypeWithCounter copy_obj{"CopyOnlyValue"};
    LOG_INFO(logger, "Test: {}", copy_obj);
  }
  logger->flush_log();

  REQUIRE_GE(CopyOnlyTypeWithCounter::copy_ctor_count.load(), 3);
  REQUIRE_LE(CopyOnlyTypeWithCounter::copy_ctor_count.load(), 4);
  REQUIRE_GE(CopyOnlyTypeWithCounter::dtor_count.load(), 4);
  REQUIRE_LE(CopyOnlyTypeWithCounter::dtor_count.load(), 5);

  // Test destruction
  MoveAndCopyTypeWithCounter::move_ctor_count = 0;
  MoveAndCopyTypeWithCounter::copy_ctor_count = 0;
  MoveAndCopyTypeWithCounter::dtor_count = 0;
  {
    MoveAndCopyTypeWithCounter both_lvalue{"BothLvalue"};
    LOG_INFO(logger, "Test: {}", both_lvalue);
    REQUIRE_EQ(both_lvalue.value, "BothLvalue");
  }
  logger->flush_log();

  REQUIRE_GE(MoveAndCopyTypeWithCounter::move_ctor_count.load(), 2);
  REQUIRE_LE(MoveAndCopyTypeWithCounter::move_ctor_count.load(), 3);
  REQUIRE_EQ(MoveAndCopyTypeWithCounter::copy_ctor_count.load(), 1);
  REQUIRE_GE(MoveAndCopyTypeWithCounter::dtor_count.load(), 4);
  REQUIRE_LE(MoveAndCopyTypeWithCounter::dtor_count.load(), 5);

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  testing::remove_file(filename);
}
