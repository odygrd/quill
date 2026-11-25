/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/BinaryWriter.h"
#include "quill/Frontend.h"
#include "quill/core/Attributes.h"
#include "quill/core/ChronoTimeUtils.h"
#include "quill/core/Filesystem.h"
#include "quill/core/FrontendOptions.h"
#include "quill/core/QuillError.h"
#include "quill/sinks/FileSink.h"
#include "quill/sinks/RotatingFileSink.h"
#include "quill/sinks/Sink.h"

#ifndef QUILL_MODULE
  #include <cstddef>
  #include <cstdint>
  #include <cstdio>
  #include <cstring>
  #include <fstream>
  #include <memory>
  #include <string>
  #include <string_view>
  #include <system_error>
  #include <type_traits>
#endif

QUILL_BEGIN_NAMESPACE

namespace detail
{
/**
 * The first 24 bytes of a pcap file. The fields are in the byte order of the machine that writes
 * the file, which a reader finds out from the magic number.
 */
struct PcapFileHeader
{
  uint32_t magic_number{0xa1b23c4d}; /**< The timestamps of the packets are in nanoseconds */
  uint16_t version_major{2};
  uint16_t version_minor{4};
  int32_t thiszone{0}; /**< The timestamps are in UTC */
  uint32_t sigfigs{0}; /**< Accuracy of the timestamps, always 0 */
  uint32_t snaplen{0}; /**< The most bytes that are kept of a packet */
  uint32_t network{0}; /**< The link type of the packets */
};

/** The 16 bytes before each packet, in the byte order of the file header */
struct PcapRecordHeader
{
  uint32_t ts_sec;   /**< When the packet was captured, in seconds since epoch */
  uint32_t ts_nsec;  /**< and the nanoseconds of that second */
  uint32_t incl_len; /**< Number of bytes of the packet that are in the file */
  uint32_t orig_len; /**< Number of bytes of the packet on the wire */
};

static_assert(sizeof(PcapFileHeader) == 24, "The pcap file header is 24 bytes");
static_assert(sizeof(PcapRecordHeader) == 16, "The pcap record header is 16 bytes");

/**
 * Checks that packets with record headers that go with `file_header` can be appended to the file.
 * A capture keeps the file header that it starts with, and its packets are read with it: the
 * byte order and the precision of the timestamps, which the magic number tells, the version, the
 * snapshot length and the link type have to be the same. The time zone and the accuracy are not
 * used by readers and may differ. A file that is empty or does not exist gets the file header
 * when the sink opens it, and a pipe or a device has no earlier capture.
 *
 * @throws QuillError if the file has content that is not such a capture, or that cannot be read
 */
inline void check_pcap_file_for_append(fs::path const& filename, PcapFileHeader const& file_header)
{
  std::error_code ec;
  auto const file_size = fs::file_size(filename, ec);

  if (ec || (file_size == 0))
  {
    return;
  }

  char existing_bytes[sizeof(PcapFileHeader)];
  std::ifstream file{filename, std::ios::binary};
  file.read(existing_bytes, sizeof(existing_bytes));

  bool is_compatible = static_cast<bool>(file);

  if (is_compatible)
  {
    PcapFileHeader existing;
    std::memcpy(&existing, existing_bytes, sizeof(existing));

    is_compatible = (existing.magic_number == file_header.magic_number) &&
      (existing.version_major == file_header.version_major) &&
      (existing.version_minor == file_header.version_minor) &&
      (existing.snaplen == file_header.snaplen) && (existing.network == file_header.network);
  }

  if (!is_compatible)
  {
    QUILL_THROW(
      QuillError{"Cannot append to a file that is not a capture with the same byte order, "
                 "timestamp precision, snapshot length and link type: " +
                 file_sink_path_string(filename)});
  }
}
} // namespace detail

QUILL_BEGIN_EXPORT

/**
 * @brief Writes network packets to pcap files asynchronously, for Wireshark, tcpdump and other
 * tools that read the format.
 *
 * The record header of a packet is built on the calling thread and copied to its queue together
 * with the packet, which is copied once. The backend worker thread writes them to the file. The
 * file sink writes the pcap file header at the start of every file, so the files of a rotating
 * sink are each a capture that can be read on its own, and a packet is not split between two.
 *
 * The files have timestamps in nanoseconds, and their fields are in the byte order of the machine.
 *
 * With FileSinkConfig::set_write_buffer_size(0), the `FILE*` of the file is unbuffered on POSIX,
 * so that the rest of a packet that a failed write cuts short is written before the next one.
 *
 * Call close() before Backend::stop() when deterministic logger removal and file closure
 * are required. The destructor performs best-effort asynchronous cleanup and does not block.
 * A file whose sink is shared, e.g. by writers that are given the same file name, is closed when
 * the last owner of the sink releases it.
 *
 * @tparam TFrontendOptions Custom frontend options if they are used application-wide.
 * quill::PcapWriter is the writer for quill::FrontendOptions.
 */
template <typename TFrontendOptions>
class PcapWriterImpl
{
public:
  using frontend_t = FrontendImpl<TFrontendOptions>;
  using clock_source_t = typename BinaryWriterImpl<TFrontendOptions>::clock_source_t;

  /** The link type of Ethernet frames. See https://www.tcpdump.org/linktypes.html for the others */
  static constexpr uint32_t link_type_ethernet{1};

  /** The snapshot length that keeps every byte of a packet */
  static constexpr uint32_t default_snapshot_length{262144};

  /** The latest timestamp of a packet: a pcap file has its seconds since epoch in 32 bits */
  static constexpr uint64_t max_timestamp_ns{(uint64_t{UINT32_MAX} * 1'000'000'000u) + 999'999'999u};

  PcapWriterImpl(PcapWriterImpl const&) = delete;
  PcapWriterImpl& operator=(PcapWriterImpl const&) = delete;
  PcapWriterImpl(PcapWriterImpl&&) = delete;
  PcapWriterImpl& operator=(PcapWriterImpl&&) = delete;

  /**
   * Constructs a writer to a pcap file.
   *
   * @param filename The name of the file to write to.
   * @param sink_config Configuration settings for the file sink. Its file header and binary
   * record framing are replaced with those of a pcap file. By default the sink appends to the
   * file, which then has to be a capture with the same file header.
   * @param link_type The link type of the packets, i.e. the protocol of their first header.
   * @param snapshot_length The most bytes that are written of a packet. A longer one is cut.
   * @param clock The clock that orders the packets among the events of the backend, see
   * BinaryWriterImpl. The timestamps in the file are the ones passed to write_packet().
   *
   * @throws QuillError if the snapshot length is 0, if the sink would append to a file that is
   * not a capture with the same byte order, timestamp precision, snapshot length and link type,
   * or if the file already has a sink that does not write the pcap file header of this writer or
   * that has a binary record framing.
   * The sink checks every file that it opens later in the same way, e.g. after the file was
   * removed, and stays closed for a file that fails the check. Writers that are given the same
   * file name share its sink, as the other writers do.
   */
  explicit PcapWriterImpl(std::string const& filename, FileSinkConfig const& sink_config = FileSinkConfig{},
                          uint32_t link_type = link_type_ethernet, uint32_t snapshot_length = default_snapshot_length,
                          clock_source_t clock = ClockSource::tsc())
    : _writer(filename, _create_sink<FileSink>(filename, sink_config, link_type, snapshot_length), clock),
      _snapshot_length(snapshot_length)
  {
  }

  /**
   * Constructs a writer to rotating pcap files. Each file starts with the pcap file header.
   *
   * @param filename The name of the file to write to.
   * @param sink_config Configuration settings for the rotating file sink. Its file header and
   * binary record framing are replaced with those of a pcap file.
   * @param link_type The link type of the packets, i.e. the protocol of their first header.
   * @param snapshot_length The most bytes that are written of a packet. A longer one is cut.
   * @param clock The clock that orders the packets among the events of the backend.
   *
   * @throws QuillError for the reasons of the constructor above
   */
  PcapWriterImpl(std::string const& filename, RotatingFileSinkConfig const& sink_config,
                 uint32_t link_type = link_type_ethernet, uint32_t snapshot_length = default_snapshot_length,
                 clock_source_t clock = ClockSource::tsc())
    : _writer(filename, _create_sink<RotatingFileSink>(filename, sink_config, link_type, snapshot_length), clock),
      _snapshot_length(snapshot_length)
  {
  }

  /**
   * The two constructors above for a file that is given by path. Use them for a path that a
   * std::string cannot spell: on Windows a std::string name is decoded with the active code
   * page, while a path built from a wide string keeps every character.
   */
  template <typename TPath, std::enable_if_t<std::is_same_v<TPath, fs::path>, int> = 0>
  explicit PcapWriterImpl(TPath const& filename, FileSinkConfig const& sink_config = FileSinkConfig{},
                          uint32_t link_type = link_type_ethernet, uint32_t snapshot_length = default_snapshot_length,
                          clock_source_t clock = ClockSource::tsc())
    : _writer(detail::file_sink_name_string(filename),
              _create_sink<FileSink>(filename, sink_config, link_type, snapshot_length), clock),
      _snapshot_length(snapshot_length)
  {
  }

  /***/
  template <typename TPath, std::enable_if_t<std::is_same_v<TPath, fs::path>, int> = 0>
  PcapWriterImpl(TPath const& filename, RotatingFileSinkConfig const& sink_config,
                 uint32_t link_type = link_type_ethernet, uint32_t snapshot_length = default_snapshot_length,
                 clock_source_t clock = ClockSource::tsc())
    : _writer(detail::file_sink_name_string(filename),
              _create_sink<RotatingFileSink>(filename, sink_config, link_type, snapshot_length), clock),
      _snapshot_length(snapshot_length)
  {
  }

  /**
   * Writes a packet of which every byte was captured. This function is thread safe.
   *
   * The bytes are copied, so `packet` may be changed or destroyed after it returns.
   *
   * @param packet the bytes of the packet, from the header of its link type
   * @param size the number of bytes of the packet
   * @param timestamp_ns when the packet was captured, in nanoseconds since epoch, e.g. the
   * hardware timestamp of the network card. At most max_timestamp_ns
   *
   * @throws QuillError if the size or the timestamp cannot be written to a pcap file
   * @return true if the packet is written to the queue, false if it is dropped
   */
  QUILL_ATTRIBUTE_HOT bool write_packet(void const* packet, size_t size, uint64_t timestamp_ns)
  {
    return write_packet(packet, size, size, timestamp_ns);
  }

  /**
   * Writes a packet of which only the first bytes were captured, e.g. by a source that has its
   * own snapshot length. The file keeps both sizes. This function is thread safe.
   *
   * @param packet the bytes of the packet that were captured, from the header of its link type
   * @param captured_size the number of bytes of `packet`
   * @param original_size the number of bytes that the packet had on the wire, not less than
   * captured_size
   * @param timestamp_ns when the packet was captured, in nanoseconds since epoch. At most
   * max_timestamp_ns
   *
   * @throws QuillError if the sizes or the timestamp cannot be written to a pcap file
   * @return true if the packet is written to the queue, false if it is dropped
   */
  QUILL_ATTRIBUTE_HOT bool write_packet(void const* packet, size_t captured_size,
                                        size_t original_size, uint64_t timestamp_ns)
  {
    if (QUILL_UNLIKELY((timestamp_ns > max_timestamp_ns) || (captured_size > original_size) ||
                       (static_cast<uint64_t>(original_size) > UINT32_MAX)))
    {
      _throw_invalid_packet(captured_size, original_size, timestamp_ns);
    }

    detail::PcapRecordHeader record_header;
    record_header.ts_sec = static_cast<uint32_t>(timestamp_ns / 1'000'000'000u);
    record_header.ts_nsec = static_cast<uint32_t>(timestamp_ns % 1'000'000'000u);
    record_header.incl_len =
      (captured_size > _snapshot_length) ? _snapshot_length : static_cast<uint32_t>(captured_size);
    record_header.orig_len = static_cast<uint32_t>(original_size);

    return _writer.write(&record_header, sizeof(record_header), packet, record_header.incl_len);
  }

  /**
   * Writes a packet of which every byte was captured, with the time of the system clock as its
   * timestamp.
   *
   * @return true if the packet is written to the queue, false if it is dropped
   */
  QUILL_ATTRIBUTE_HOT bool write_packet(void const* packet, size_t size)
  {
    return write_packet(packet, size, size, detail::get_system_time_ns());
  }

  /**
   * @brief Removes the logger of this writer synchronously. The file is closed before this
   * function returns, unless something else still holds its sink, e.g. another writer of the
   * same file: the file is then closed when the last owner of the sink releases it.
   * @note This function must only be used while the backend worker is running.
   *
   * After calling close(), this writer must no longer be used.
   */
  void close() { _writer.close(); }

  /**
   * @brief Blocks the caller thread until the packets written so far are flushed to the file.
   */
  void flush() { _writer.flush(); }

private:
  /**
   * Creates the file sink of the capture, or gets the one that the file has: writers that are
   * given the same file name share its sink. The sink writes the pcap file header at the start
   * of every file, and checks that the packets of this writer can follow what is in a file that
   * it opens.
   */
  template <typename TFileSink, typename TFilename, typename TSinkConfig>
  QUILL_NODISCARD static std::shared_ptr<Sink> _create_sink(TFilename const& filename, TSinkConfig sink_config,
                                                            uint32_t link_type, uint32_t snapshot_length)
  {
    if (snapshot_length == 0)
    {
      QUILL_THROW(QuillError{"The snapshot length of a pcap file must be greater than 0: " +
                             detail::file_sink_name_string(filename)});
    }

    detail::PcapFileHeader file_header;
    file_header.snaplen = snapshot_length;
    file_header.network = link_type;

    std::string_view const file_header_bytes{reinterpret_cast<char const*>(&file_header), sizeof(file_header)};
    sink_config.set_file_header(file_header_bytes);

    // The record header of a packet is its frame
    sink_config.set_binary_record_framing(BinaryRecordFraming::None);

    // The sink checks each file that it opens, with the file as it was opened: the path can have
    // another capture by the time the sink opens it again, e.g. after the file was removed
    FileEventNotifier file_event_notifier;

#if defined(_WIN32)
    file_event_notifier.after_open = [file_header](fs::path const& file_path, FileEventNotifierHandle)
    { detail::check_pcap_file_for_append(file_path, file_header); };
#else
    // Without the write buffer the packets go through the FILE*. Unbuffered, it takes only what
    // reaches the file, so that the rest of a packet that a write cuts short is kept for the next
    // flush, as with a binary record framing
    bool const is_unbuffered = (sink_config.write_buffer_size() == 0);

    file_event_notifier.after_open =
      [file_header, is_unbuffered](fs::path const& file_path, FileEventNotifierHandle file)
    {
      if (is_unbuffered)
      {
        std::setvbuf(file, nullptr, _IONBF, 0);
      }

      detail::check_pcap_file_for_append(file_path, file_header);
    };
#endif

    std::shared_ptr<TFileSink> sink =
      frontend_t::template create_or_get_sink<TFileSink>(filename, sink_config, file_event_notifier);

    // A sink that exists for the file keeps the configuration that it was created with
    if ((sink->config().file_header() != sink_config.file_header()) ||
        (sink->config().binary_record_framing() != sink_config.binary_record_framing()))
    {
      QUILL_THROW(QuillError{
        "The file already has a sink that does not write the pcap file header of this writer, or "
        "that frames its records: " +
        detail::file_sink_name_string(filename)});
    }

    return sink;
  }

  /** Not inlined: it keeps the message of the error out of the callers of write_packet() */
  QUILL_NOINLINE QUILL_ATTRIBUTE_COLD static void _throw_invalid_packet(size_t captured_size, size_t original_size,
                                                                        uint64_t timestamp_ns)
  {
    QUILL_THROW(QuillError{"A pcap file cannot hold a packet of " + std::to_string(original_size) +
                           " bytes of which " + std::to_string(captured_size) +
                           " were captured at " + std::to_string(timestamp_ns) + " ns since epoch"});
  }

  BinaryWriterImpl<TFrontendOptions> _writer;
  uint32_t _snapshot_length;
};

using PcapWriter = PcapWriterImpl<FrontendOptions>;

QUILL_END_EXPORT

QUILL_END_NAMESPACE
