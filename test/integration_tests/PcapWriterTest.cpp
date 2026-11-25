#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/PcapWriter.h"
#include "quill/sinks/FileSink.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#if defined(__linux__)
  #include <csignal>
  #include <sys/resource.h>
#endif

using namespace quill;

namespace
{
struct PcapPacket
{
  uint64_t timestamp_ns{0};
  uint32_t original_size{0};
  std::string bytes;
};

struct PcapFile
{
  bool valid{false};
  int32_t thiszone{0};
  uint32_t snapshot_length{0};
  uint32_t link_type{0};
  std::vector<PcapPacket> packets;
};

template <typename T>
T read_value(std::string const& contents, size_t& position)
{
  T value;
  std::memcpy(&value, contents.data() + position, sizeof(value));
  position += sizeof(value);
  return value;
}

/** Reads a pcap file as a tool does: the file header and then each record header and packet */
PcapFile read_pcap_file(char const* filename)
{
  std::string contents;

  {
    std::ifstream file{filename, std::ios::binary};
    contents.assign(std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{});
  }

  PcapFile pcap_file;

  if (contents.size() < 24)
  {
    return pcap_file;
  }

  size_t position{0};
  uint32_t const magic_number = read_value<uint32_t>(contents, position);
  uint16_t const version_major = read_value<uint16_t>(contents, position);
  uint16_t const version_minor = read_value<uint16_t>(contents, position);
  int32_t const thiszone = read_value<int32_t>(contents, position);
  uint32_t const sigfigs = read_value<uint32_t>(contents, position);
  pcap_file.snapshot_length = read_value<uint32_t>(contents, position);
  pcap_file.link_type = read_value<uint32_t>(contents, position);

  // The magic number of a file with timestamps in nanoseconds, in the byte order of this machine
  pcap_file.valid =
    (magic_number == 0xa1b23c4d) && (version_major == 2) && (version_minor == 4) && (sigfigs == 0);
  pcap_file.thiszone = thiszone;

  while (pcap_file.valid && (position < contents.size()))
  {
    if ((contents.size() - position) < 16)
    {
      pcap_file.valid = false;
      break;
    }

    uint32_t const ts_sec = read_value<uint32_t>(contents, position);
    uint32_t const ts_nsec = read_value<uint32_t>(contents, position);
    uint32_t const incl_len = read_value<uint32_t>(contents, position);
    uint32_t const orig_len = read_value<uint32_t>(contents, position);

    if ((contents.size() - position) < incl_len)
    {
      pcap_file.valid = false;
      break;
    }

    PcapPacket packet;
    packet.timestamp_ns = (static_cast<uint64_t>(ts_sec) * 1'000'000'000u) + ts_nsec;
    packet.original_size = orig_len;
    packet.bytes.assign(contents, position, incl_len);
    position += incl_len;

    pcap_file.packets.push_back(std::move(packet));
  }

  return pcap_file;
}

/** Creates a capture without packets, as another tool or an earlier run leaves it */
void create_pcap_file(char const* filename, uint32_t magic_number, uint32_t snapshot_length,
                      uint32_t link_type, int32_t thiszone = 0)
{
  uint16_t const version_major{2};
  uint16_t const version_minor{4};
  uint32_t const sigfigs{0};

  std::ofstream file{filename, std::ios::binary};
  file.write(reinterpret_cast<char const*>(&magic_number), sizeof(magic_number));
  file.write(reinterpret_cast<char const*>(&version_major), sizeof(version_major));
  file.write(reinterpret_cast<char const*>(&version_minor), sizeof(version_minor));
  file.write(reinterpret_cast<char const*>(&thiszone), sizeof(thiszone));
  file.write(reinterpret_cast<char const*>(&sigfigs), sizeof(sigfigs));
  file.write(reinterpret_cast<char const*>(&snapshot_length), sizeof(snapshot_length));
  file.write(reinterpret_cast<char const*>(&link_type), sizeof(link_type));
}
} // namespace

/***/
TEST_CASE("pcap_writer")
{
  static constexpr char const* filename = "pcap_writer.pcap";
  static constexpr char const* rotating_filename = "pcap_writer_rotating.pcap";
  static constexpr char const* rotating_filename_1 = "pcap_writer_rotating.1.pcap";
  static constexpr char const* rotating_filename_2 = "pcap_writer_rotating.2.pcap";
  static constexpr char const* appended_filename = "pcap_writer_appended.pcap";
  static constexpr char const* microseconds_filename = "pcap_writer_microseconds.pcap";
  static constexpr char const* link_type_filename = "pcap_writer_link_type.pcap";
  static constexpr char const* plain_sink_filename = "pcap_writer_plain_sink.pcap";
  static constexpr char const* framed_sink_filename = "pcap_writer_framed_sink.pcap";
  static constexpr char const* packets_only_filename = "pcap_writer_packets_only.pcap";
  static constexpr char const* legacy_filename = "pcap_writer_legacy.pcap";
  static constexpr char const* write_only_filename = "pcap_writer_write_only.pcap";
  static constexpr char const* reopened_directory = "pcap_writer_reopened";
  QUILL_MAYBE_UNUSED static constexpr char const* reopened_filename =
    "pcap_writer_reopened/capture.pcap";
  static constexpr char const* short_write_filename = "pcap_writer_short_write.pcap";

  testing::remove_file(legacy_filename);
  testing::remove_file(write_only_filename);
  fs::remove_all(reopened_directory);
  testing::remove_file(short_write_filename);

  testing::remove_file(appended_filename);
  testing::remove_file(microseconds_filename);
  testing::remove_file(link_type_filename);
  testing::remove_file(plain_sink_filename);
  testing::remove_file(framed_sink_filename);
  testing::remove_file(packets_only_filename);
  testing::remove_file(filename);
  testing::remove_file(rotating_filename);
  testing::remove_file(rotating_filename_1);
  testing::remove_file(rotating_filename_2);

  // Packets with bytes that a text file would not keep
  std::string const first_packet{"\x01\x00\x5e\n\r\n\xff", 7};
  std::string const second_packet(200, '\n');
  std::string const long_packet(300, 'L');

  uint64_t const first_timestamp{1'686'528'000'000'000'001ull};
  uint64_t const second_timestamp{1'686'528'001'999'999'999ull};

  std::atomic<size_t> error_notifications{0};
  BackendOptions backend_options;
  backend_options.error_notifier = [&error_notifications](std::string const& message)
  {
    if (message.find("Allocated a new SPSC queue") == std::string::npos)
    {
      error_notifications.fetch_add(1, std::memory_order_relaxed);
    }
  };

  Backend::start(backend_options);

  {
    FileSinkConfig sink_config;
    sink_config.set_open_mode('w');

    // The file header of a pcap file replaces these
    sink_config.set_file_header("not a pcap header");
    sink_config.set_binary_record_framing(BinaryRecordFraming::Size);

    // Packets of a custom link type, of which at most 256 bytes are kept
    PcapWriter writer{filename, sink_config, 147, 256};
    REQUIRE(writer.write_packet(first_packet.data(), first_packet.size(), first_timestamp));
    REQUIRE(writer.write_packet(second_packet.data(), second_packet.size(), second_timestamp));
    REQUIRE(writer.write_packet(long_packet.data(), long_packet.size(), second_timestamp));

    writer.flush();
    REQUIRE_EQ(read_pcap_file(filename).packets.size(), 3);

    // Without a timestamp, the packet has the time of the system clock
    REQUIRE(writer.write_packet(first_packet.data(), first_packet.size()));

    // A source that kept the first 96 bytes of a packet of 1500: the file has both sizes
    REQUIRE(writer.write_packet(long_packet.data(), 96, 1500, second_timestamp));

    // The latest time that a pcap file can hold
    REQUIRE(writer.write_packet(first_packet.data(), first_packet.size(), PcapWriter::max_timestamp_ns));

#if !defined(QUILL_NO_EXCEPTIONS)
    // What the fields of a record header cannot hold is rejected, not written as something else
    REQUIRE_THROWS_AS(
      writer.write_packet(first_packet.data(), first_packet.size(), PcapWriter::max_timestamp_ns + 1u), QuillError);
    REQUIRE_THROWS_AS(writer.write_packet(long_packet.data(), 96, 95, second_timestamp), QuillError);

  #if SIZE_MAX > UINT32_MAX
    REQUIRE_THROWS_AS(writer.write_packet(long_packet.data(), 96, static_cast<size_t>(UINT32_MAX) + 1u, second_timestamp),
                      QuillError);
  #endif
#endif

    writer.close();

#if !defined(QUILL_NO_EXCEPTIONS)
    REQUIRE_THROWS_AS(writer.write_packet(first_packet.data(), first_packet.size()), QuillError);
#endif
  }

  {
    // Each packet takes 216 bytes with its record header, so two fit in a file of 512 bytes
    // after the file header of 24 bytes, and the third one starts the next file
    RotatingFileSinkConfig sink_config;
    sink_config.set_open_mode('w');
    sink_config.set_rotation_max_file_size(512);

    PcapWriter writer{rotating_filename, sink_config};

    for (uint64_t i = 0; i < 3; ++i)
    {
      REQUIRE(writer.write_packet(second_packet.data(), second_packet.size(), first_timestamp + i));
    }

    writer.close();
  }

  {
    // By default the writer appends. A capture with the same file header gets the packets
    create_pcap_file(appended_filename, 0xa1b23c4d, PcapWriter::default_snapshot_length,
                     PcapWriter::link_type_ethernet);

    PcapWriter writer{appended_filename};
    REQUIRE(writer.write_packet(first_packet.data(), first_packet.size(), first_timestamp));
    writer.flush();

    // A second writer that is given the name shares the sink of the file, as the other writers
    // do, and its packets follow in the same capture. The file can also be given by path
    PcapWriter second_writer{fs::path{appended_filename}};
    REQUIRE(second_writer.write_packet(second_packet.data(), second_packet.size(), second_timestamp));

    writer.close();
    second_writer.close();
  }

  {
    // A log statement through the logger of the writer is not written to the capture: one of an
    // application that logs to the first valid logger, which is the logger of the writer when
    // there is no other, or to every logger it finds. Either can still flush with it
    PcapWriter writer{packets_only_filename};
    REQUIRE(writer.write_packet(first_packet.data(), first_packet.size(), first_timestamp));
    writer.flush();

    uintmax_t const file_size = fs::file_size(packets_only_filename);

    Logger* const valid_logger = Frontend::get_valid_logger();
    REQUIRE(valid_logger);
    LOG_INFO(valid_logger, "text that is not a packet");
    valid_logger->flush_log();

    for (Logger* logger : Frontend::get_all_loggers())
    {
      LOG_INFO(logger, "text that is not a packet");
      LOG_CRITICAL(logger, "text that is not a packet");
      logger->flush_log();
    }

    REQUIRE_EQ(fs::file_size(packets_only_filename), file_size);
    writer.close();
  }

  {
    // A capture of an older tool has a time zone in its file header, which no reader uses. It
    // is the same format, and the writer appends to it
    create_pcap_file(legacy_filename, 0xa1b23c4d, PcapWriter::default_snapshot_length,
                     PcapWriter::link_type_ethernet, 3600);

    PcapWriter writer{legacy_filename};
    REQUIRE(writer.write_packet(first_packet.data(), first_packet.size(), first_timestamp));
    writer.close();
  }

#if !defined(_WIN32)
  {
    // A file that can be written and not read is truncated and gets its file header from the
    // sink, so there is nothing in it to check
    testing::create_file(write_only_filename, "to be truncated");
    fs::permissions(write_only_filename, fs::perms::owner_write);

    FileSinkConfig sink_config;
    sink_config.set_open_mode('w');

    PcapWriter writer{write_only_filename, sink_config};
    REQUIRE(writer.write_packet(first_packet.data(), first_packet.size(), first_timestamp));
    writer.close();

    fs::permissions(write_only_filename, fs::perms::owner_read | fs::perms::owner_write);
  }
#endif

#if !defined(QUILL_NO_EXCEPTIONS)
  {
    // A capture with timestamps in microseconds would read the nanoseconds of appended packets
    // as microseconds, so the writer refuses it, as it does one of another link type
    create_pcap_file(microseconds_filename, 0xa1b2c3d4, PcapWriter::default_snapshot_length,
                     PcapWriter::link_type_ethernet);
    REQUIRE_THROWS_AS(PcapWriter{microseconds_filename}, QuillError);
    REQUIRE_EQ(fs::file_size(microseconds_filename), 24u);

    create_pcap_file(link_type_filename, 0xa1b23c4d, PcapWriter::default_snapshot_length, 147);
    REQUIRE_THROWS_AS(PcapWriter{link_type_filename}, QuillError);
    REQUIRE_EQ(fs::file_size(link_type_filename), 24u);

    // A capture that keeps no byte of its packets
    REQUIRE_THROWS_AS(PcapWriter(link_type_filename, FileSinkConfig{}, 147, 0), QuillError);
    REQUIRE_THROWS_AS(PcapWriter(fs::path{link_type_filename}, RotatingFileSinkConfig{}, 147, 0), QuillError);
  }

  {
    // The file has a sink that was created for something else, which does not start its files
    // with the pcap file header, so the writer does not share it
    auto const plain_sink = Frontend::create_or_get_sink<FileSink>(plain_sink_filename, FileSinkConfig{});
    REQUIRE(plain_sink);
    REQUIRE_THROWS_AS(PcapWriter{plain_sink_filename}, QuillError);
  }

  {
    // The file has a sink that starts its files with the pcap file header, but writes the size
    // of each record before it. A reader would take that size for the start of a packet, so the
    // writer does not share the sink
    detail::PcapFileHeader file_header;
    file_header.snaplen = PcapWriter::default_snapshot_length;
    file_header.network = PcapWriter::link_type_ethernet;

    FileSinkConfig framed_sink_config;
    framed_sink_config.set_file_header(
      std::string_view{reinterpret_cast<char const*>(&file_header), sizeof(file_header)});
    framed_sink_config.set_binary_record_framing(BinaryRecordFraming::Size);

    auto const framed_sink = Frontend::create_or_get_sink<FileSink>(framed_sink_filename, framed_sink_config);
    REQUIRE(framed_sink);
    REQUIRE_THROWS_AS(PcapWriter{framed_sink_filename}, QuillError);
  }
#endif

// A file that cannot be opened ends the process when exceptions are disabled
#if !defined(_WIN32) && !defined(QUILL_NO_EXCEPTIONS)
  {
    // The sink makes the check again for every file it opens. Here its directory goes away and
    // comes back with a capture in microseconds at the path: the sink does not append to it
    fs::create_directories(reopened_directory);

    PcapWriter writer{reopened_filename};
    REQUIRE(writer.write_packet(first_packet.data(), first_packet.size(), first_timestamp));
    writer.flush();

    fs::remove_all(reopened_directory);
    REQUIRE(writer.write_packet(first_packet.data(), first_packet.size(), first_timestamp));
    writer.flush();

    fs::create_directories(reopened_directory);
    create_pcap_file(reopened_filename, 0xa1b2c3d4, PcapWriter::default_snapshot_length,
                     PcapWriter::link_type_ethernet);

    REQUIRE(writer.write_packet(first_packet.data(), first_packet.size(), first_timestamp));
    writer.flush();
    REQUIRE(writer.write_packet(first_packet.data(), first_packet.size(), first_timestamp));
    writer.flush();
    writer.close();

    REQUIRE_EQ(fs::file_size(reopened_filename), 24u);
    REQUIRE_GE(error_notifications.load(std::memory_order_relaxed), 1u);
  }
#endif

#if defined(__linux__)
  {
    // Without the write buffer, a write past the file size limit of the process cuts a packet
    // short. The rest of it is written once the limit is lifted, before the next packet. A
    // buffered FILE* discarded it, so that the next packet was read as a part of the first
    FileSinkConfig sink_config;
    sink_config.set_open_mode('w');
    sink_config.set_write_buffer_size(0);

    PcapWriter writer{short_write_filename, sink_config};
    std::string const short_written_packet(1500, 'S');

    rlimit previous_limit{};
    REQUIRE_EQ(::getrlimit(RLIMIT_FSIZE, &previous_limit), 0);

    // The process is not ended when a write goes past the limit, which then fails with EFBIG
    auto const previous_handler = std::signal(SIGXFSZ, SIG_IGN);

    rlimit limit = previous_limit;
    limit.rlim_cur = 1024;
    REQUIRE_EQ(::setrlimit(RLIMIT_FSIZE, &limit), 0);

    REQUIRE(writer.write_packet(short_written_packet.data(), short_written_packet.size(), first_timestamp));
    writer.flush();

    REQUIRE_EQ(::setrlimit(RLIMIT_FSIZE, &previous_limit), 0);
    std::signal(SIGXFSZ, previous_handler);

    REQUIRE(writer.write_packet(first_packet.data(), first_packet.size(), second_timestamp));
    writer.close();

    PcapFile const short_write_pcap_file = read_pcap_file(short_write_filename);
    REQUIRE(short_write_pcap_file.valid);
    REQUIRE_EQ(short_write_pcap_file.packets.size(), 2);
    REQUIRE(short_write_pcap_file.packets[0].bytes == short_written_packet);
    REQUIRE(short_write_pcap_file.packets[1].bytes == first_packet);
  }
#endif

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // The packets of the two writers that shared the sink of the file
  PcapFile const appended_pcap_file = read_pcap_file(appended_filename);
  REQUIRE(appended_pcap_file.valid);
  REQUIRE_EQ(appended_pcap_file.packets.size(), 2);
  REQUIRE_EQ(appended_pcap_file.packets[0].timestamp_ns, first_timestamp);
  REQUIRE_EQ(appended_pcap_file.packets[1].timestamp_ns, second_timestamp);

  PcapFile const legacy_pcap_file = read_pcap_file(legacy_filename);
  REQUIRE(legacy_pcap_file.valid);
  REQUIRE_EQ(legacy_pcap_file.thiszone, 3600);
  REQUIRE_EQ(legacy_pcap_file.packets.size(), 1);
  REQUIRE(legacy_pcap_file.packets[0].bytes == first_packet);

#if !defined(_WIN32)
  PcapFile const write_only_pcap_file = read_pcap_file(write_only_filename);
  REQUIRE(write_only_pcap_file.valid);
  REQUIRE_EQ(write_only_pcap_file.packets.size(), 1);
#endif

  PcapFile const pcap_file = read_pcap_file(filename);
  REQUIRE(pcap_file.valid);
  REQUIRE_EQ(pcap_file.link_type, 147);
  REQUIRE_EQ(pcap_file.snapshot_length, 256);
  REQUIRE_EQ(pcap_file.packets.size(), 6);

  REQUIRE_EQ(pcap_file.packets[0].timestamp_ns, first_timestamp);
  REQUIRE_EQ(pcap_file.packets[0].original_size, first_packet.size());
  REQUIRE(pcap_file.packets[0].bytes == first_packet);

  REQUIRE_EQ(pcap_file.packets[1].timestamp_ns, second_timestamp);
  REQUIRE_EQ(pcap_file.packets[1].original_size, second_packet.size());
  REQUIRE(pcap_file.packets[1].bytes == second_packet);

  // The packet that is longer than the snapshot length is cut, and keeps its size on the wire
  REQUIRE_EQ(pcap_file.packets[2].original_size, long_packet.size());
  REQUIRE(pcap_file.packets[2].bytes == long_packet.substr(0, 256));

  REQUIRE_GT(pcap_file.packets[3].timestamp_ns, second_timestamp);
  REQUIRE(pcap_file.packets[3].bytes == first_packet);

  REQUIRE_EQ(pcap_file.packets[4].original_size, 1500);
  REQUIRE(pcap_file.packets[4].bytes == long_packet.substr(0, 96));

  REQUIRE_EQ(pcap_file.packets[5].timestamp_ns, PcapWriter::max_timestamp_ns);

  // Every file of a rotating sink is a capture of its own
  PcapFile const rotated_pcap_file = read_pcap_file(rotating_filename_1);
  REQUIRE(rotated_pcap_file.valid);
  REQUIRE_EQ(rotated_pcap_file.link_type, PcapWriter::link_type_ethernet);
  REQUIRE_EQ(rotated_pcap_file.snapshot_length, PcapWriter::default_snapshot_length);
  REQUIRE_EQ(rotated_pcap_file.packets.size(), 2);
  REQUIRE_EQ(rotated_pcap_file.packets[0].timestamp_ns, first_timestamp);
  REQUIRE_EQ(rotated_pcap_file.packets[1].timestamp_ns, first_timestamp + 1);

  PcapFile const active_pcap_file = read_pcap_file(rotating_filename);
  REQUIRE(active_pcap_file.valid);
  REQUIRE_EQ(active_pcap_file.packets.size(), 1);
  REQUIRE_EQ(active_pcap_file.packets[0].timestamp_ns, first_timestamp + 2);
  REQUIRE(active_pcap_file.packets[0].bytes == second_packet);

  REQUIRE_FALSE(fs::exists(rotating_filename_2));

  testing::remove_file(filename);
  testing::remove_file(rotating_filename);
  testing::remove_file(rotating_filename_1);
  testing::remove_file(appended_filename);
  testing::remove_file(microseconds_filename);
  testing::remove_file(link_type_filename);
  testing::remove_file(plain_sink_filename);
  testing::remove_file(framed_sink_filename);
  testing::remove_file(packets_only_filename);
  testing::remove_file(legacy_filename);
  testing::remove_file(write_only_filename);
  fs::remove_all(reopened_directory);
  testing::remove_file(short_write_filename);
}
