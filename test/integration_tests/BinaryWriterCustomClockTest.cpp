#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/BinaryWriter.h"
#include "quill/Frontend.h"
#include "quill/PcapWriter.h"
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
struct ReplayClock : UserClockSource
{
  uint64_t now() const noexcept override { return timestamp.load(std::memory_order_relaxed); }

  std::atomic<uint64_t> timestamp{0};
};

struct ReplayClockFrontendOptions : FrontendOptions
{
  using clock_type = ReplayClock;
};

struct TimestampCapturingSink final : public Sink
{
  void write_log(LogRecord const& /* record */) override {}

  void write_binary(BinaryRecord const& record) override { timestamps.push_back(record.timestamp); }

  void flush_sink(SinkFlushReason) override {}

  std::vector<uint64_t> timestamps;
};
} // namespace

/***/
TEST_CASE("binary_writer_custom_clock")
{
  static constexpr char const* pcap_filename = "binary_writer_custom_clock.pcap";
  testing::remove_file(pcap_filename);

  using ReplayFrontend = FrontendImpl<ReplayClockFrontendOptions>;

  ReplayClock clock;

  Backend::start();

  auto sink =
    ReplayFrontend::create_or_get_sink<TimestampCapturingSink>("binary_writer_custom_clock_sink");

  {
    // With a clock type in the frontend options, a writer is given the clock of its records
    BinaryWriterImpl<ReplayClockFrontendOptions> writer{"binary_writer_custom_clock", sink,
                                                        ClockSource::custom(clock)};

    clock.timestamp.store(1'686'528'000'000'000'000ull, std::memory_order_relaxed);
    REQUIRE(writer.write("first", 5));

    clock.timestamp.store(1'686'528'060'000'000'000ull, std::memory_order_relaxed);
    REQUIRE(writer.write("second", 6));

    writer.close();
  }

  {
    FileSinkConfig sink_config;
    sink_config.set_open_mode('w');

    PcapWriterImpl<ReplayClockFrontendOptions> writer{
      pcap_filename, sink_config, PcapWriterImpl<ReplayClockFrontendOptions>::link_type_ethernet,
      PcapWriterImpl<ReplayClockFrontendOptions>::default_snapshot_length, ClockSource::custom(clock)};

    REQUIRE(writer.write_packet("packet", 6, 1'686'528'060'000'000'000ull));
    writer.close();
  }

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // The records have the time of the clock, not of the machine
  REQUIRE_EQ(sink->timestamps.size(), 2);
  REQUIRE_EQ(sink->timestamps[0], 1'686'528'000'000'000'000ull);
  REQUIRE_EQ(sink->timestamps[1], 1'686'528'060'000'000'000ull);

  // The file header, a record header and the packet
  REQUIRE_EQ(fs::file_size(pcap_filename), 24u + 16u + 6u);

  testing::remove_file(pcap_filename);
}
