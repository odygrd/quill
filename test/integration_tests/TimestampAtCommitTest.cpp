#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogFunctions.h"
#include "quill/LogMacros.h"
#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"
#include "quill/sinks/FileSink.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace
{
/** Numbers what a log call does in the order it does it */
uint64_t next_step()
{
  static std::atomic<uint64_t> step{0};
  return step.fetch_add(1, std::memory_order_relaxed) + 1;
}

/** A clock whose timestamp is the step at which it is read, in nanoseconds since epoch */
struct StepClock : quill::UserClockSource
{
  uint64_t now() const noexcept override { return next_step(); }
};

struct StepClockOptions : quill::FrontendOptions
{
  using clock_type = StepClock;
  static constexpr quill::QueueType queue_type = quill::QueueType::BoundedDropping;
};
} // namespace

/** An argument that is queued and formatted as the step at which it is encoded */
struct EncodeStep
{
};

template <>
struct quill::Codec<EncodeStep>
{
  static size_t compute_encoded_size(SizeCacheVector& /* conditional_arg_size_cache */,
                                     ::EncodeStep const& /* arg */) noexcept
  {
    return sizeof(uint64_t);
  }

  static void encode(std::byte*& buffer, SizeCacheVector const& /* conditional_arg_size_cache */,
                     uint32_t& /* conditional_arg_size_cache_index */, ::EncodeStep const& /* arg */) noexcept
  {
    uint64_t const step = next_step();
    std::memcpy(buffer, &step, sizeof(step));
    buffer += sizeof(step);
  }

  static uint64_t decode_arg(std::byte*& buffer)
  {
    uint64_t step;
    std::memcpy(&step, buffer, sizeof(step));
    buffer += sizeof(step);
    return step;
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};

/***/
TEST_CASE("timestamp_at_commit")
{
  static constexpr char const* filename = "timestamp_at_commit.log";
  using StepFrontend = quill::FrontendImpl<StepClockOptions>;

  StepClock clock;

  quill::FileSinkConfig config;
  config.set_open_mode('w');
  auto sink = StepFrontend::create_or_get_sink<quill::FileSink>(filename, config);

  StepFrontend::logger_t* logger = StepFrontend::create_or_get_logger(
    "timestamp_at_commit_logger", sink,
    quill::PatternFormatterOptions{"%(time) %(message)", "%Y-%m-%d %H:%M:%S.%Qns", quill::Timezone::GmtTime},
    quill::ClockSource::custom(clock));

  // The clock is read after the argument is encoded, just before the record is committed to the
  // queue, so the timestamp of each record is one step after its argument.

  // The first call of a thread creates its queue, in the noinline helper of log_statement()
  LOG_INFO(logger, "encoded at step {}", EncodeStep{});

  // An oversized record is dropped before reading the clock. The next record still starts at step 3.
  std::string const oversized(StepClockOptions::initial_queue_capacity, 'x');
  LOG_INFO(logger, "{}", oversized);

  // The inlined path of log_statement()
  LOG_INFO(logger, "encoded at step {}", EncodeStep{});

  // log_statement_runtime_metadata()
  quill::info(logger, "encoded at step {}", EncodeStep{});

  quill::Backend::start();
  logger->flush_log();
  StepFrontend::remove_logger(logger);
  quill::Backend::stop();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 3);
  REQUIRE_EQ(file_contents[0], "1970-01-01 00:00:00.000000002 encoded at step 1");
  REQUIRE_EQ(file_contents[1], "1970-01-01 00:00:00.000000004 encoded at step 3");
  REQUIRE_EQ(file_contents[2], "1970-01-01 00:00:00.000000006 encoded at step 5");

  quill::testing::remove_file(filename);
}
