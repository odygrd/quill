#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/CsvWriter.h"
#include "quill/Frontend.h"
#include "quill/UserClockSource.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using namespace quill;

namespace
{
/** A clock of the application, e.g. the time of the market data that it replays */
struct CsvReplayClock : UserClockSource
{
  uint64_t now() const noexcept override { return timestamp.load(std::memory_order_relaxed); }

  std::atomic<uint64_t> timestamp{0};
};

struct CsvReplayClockFrontendOptions : FrontendOptions
{
  using clock_type = CsvReplayClock;
};

struct CsvReplayClockSchema
{
  static constexpr char const* header = "id,name";
  static constexpr char const* format = "{},{}";
};

struct CsvRowCapturingSink final : public Sink
{
  void write_log(LogRecord const& record) override
  {
    rows.emplace_back(record.log_message);
    timestamps.push_back(record.log_timestamp);
  }

  void flush_sink(SinkFlushReason) override {}

  std::vector<std::string> rows;
  std::vector<uint64_t> timestamps;
};
} // namespace

/***/
TEST_CASE("csv_writer_custom_clock")
{
  static constexpr char const* filename = "csv_writer_custom_clock.csv";
  testing::remove_file(filename);

  using ReplayFrontend = FrontendImpl<CsvReplayClockFrontendOptions>;
  using ReplayCsvWriter = CsvWriter<CsvReplayClockSchema, CsvReplayClockFrontendOptions>;

  CsvReplayClock clock;
  clock.timestamp.store(1'686'528'000'000'000'000ull, std::memory_order_relaxed);

  Backend::start();

  auto sink =
    ReplayFrontend::create_or_get_sink<CsvRowCapturingSink>("csv_writer_custom_clock_sink");

  {
    // With a clock type in the frontend options, a writer is given the clock of its rows
    ReplayCsvWriter writer{"csv_writer_custom_clock", sink, true, ClockSource::custom(clock)};

    clock.timestamp.store(1'686'528'060'000'000'000ull, std::memory_order_relaxed);
    writer.append_row(1, "first");
    writer.close();
  }

  {
    FileSinkConfig sink_config;
    sink_config.set_open_mode('w');

    ReplayCsvWriter writer{filename, sink_config, true, ClockSource::custom(clock)};
    writer.append_row(2, "second");
    writer.close();
  }

  {
    // The file of the writer above is appended to
    ReplayCsvWriter writer{filename, 'a', FilenameAppendOption::None, ClockSource::custom(clock)};
    writer.append_row(3, "third");
    writer.close();
  }

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // The header and the row have the time of the clock, not of the machine
  REQUIRE_EQ(sink->rows.size(), 2);
  REQUIRE_EQ(sink->rows[0], "id,name");
  REQUIRE_EQ(sink->rows[1], "1,first");
  REQUIRE_EQ(sink->timestamps[0], 1'686'528'000'000'000'000ull);
  REQUIRE_EQ(sink->timestamps[1], 1'686'528'060'000'000'000ull);

  std::vector<std::string> const expected{"id,name", "2,second", "3,third"};
  REQUIRE(testing::file_contents(filename) == expected);

  testing::remove_file(filename);
}
