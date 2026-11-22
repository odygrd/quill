#include "quill/Backend.h"
#include "quill/PcapWriter.h"
#include "quill/sinks/RotatingFileSink.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>

/**
 * This example writes network packets to pcap files that Wireshark and tcpdump can read, and
 * starts a new file every hour.
 *
 * A pcap file is a file header followed by the packets, each after a record header with its
 * timestamp and size:
 *
 *   [file header] [record header] [packet] [record header] [packet] ...
 *
 * quill::PcapWriter writes it with two things that any binary format can use. The file sink
 * writes the file header at the start of every file it creates, see
 * FileSinkConfig::set_file_header(). The record header is built on the calling thread and copied
 * to the queue together with the packet as one binary record, see BinaryWriter::write(), so
 * the packet is copied once and a rotation does not come between the two.
 *
 * To read the capture:
 *
 *   tcpdump -nn -r quill_capture.pcap
 *   wireshark quill_capture.pcap
 */

/** The headers of a packet are in network byte order */
void append_uint16(unsigned char*& position, uint16_t value)
{
  *position++ = static_cast<unsigned char>(value >> 8);
  *position++ = static_cast<unsigned char>(value & 0xff);
}

void append_bytes(unsigned char*& position, void const* bytes, size_t size)
{
  std::memcpy(position, bytes, size);
  position += size;
}

uint16_t ipv4_header_checksum(unsigned char const* header, size_t size)
{
  uint32_t sum{0};

  for (size_t i = 0; i < size; i += 2)
  {
    sum += static_cast<uint32_t>((header[i] << 8) | header[i + 1]);
  }

  while ((sum >> 16) != 0)
  {
    sum = (sum & 0xffff) + (sum >> 16);
  }

  return static_cast<uint16_t>(~sum);
}

/**
 * Stands in for a packet from the network: an Ethernet frame with a UDP datagram for a multicast
 * group, as a market data feed sends them.
 * @return the size of the packet
 */
size_t make_udp_packet(unsigned char* packet, uint16_t sequence, std::string_view payload)
{
  static constexpr unsigned char destination_mac[6]{0x01, 0x00, 0x5e, 0x01, 0x01, 0x01};
  static constexpr unsigned char source_mac[6]{0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
  static constexpr unsigned char source_ip[4]{10, 0, 0, 1};
  static constexpr unsigned char destination_ip[4]{239, 1, 1, 1};
  static constexpr size_t ipv4_header_size{20};
  static constexpr size_t udp_header_size{8};

  unsigned char* position = packet;

  // Ethernet header
  append_bytes(position, destination_mac, sizeof(destination_mac));
  append_bytes(position, source_mac, sizeof(source_mac));
  append_uint16(position, 0x0800); // IPv4

  // IPv4 header
  unsigned char* const ipv4_header = position;
  *position++ = 0x45; // version 4, header of 5 words
  *position++ = 0x00;
  append_uint16(position, static_cast<uint16_t>(ipv4_header_size + udp_header_size + payload.size()));
  append_uint16(position, sequence); // identification
  append_uint16(position, 0x4000);   // do not fragment
  *position++ = 64;                  // time to live
  *position++ = 17;                  // UDP
  append_uint16(position, 0);        // checksum, filled in below
  append_bytes(position, source_ip, sizeof(source_ip));
  append_bytes(position, destination_ip, sizeof(destination_ip));

  unsigned char* checksum_position = ipv4_header + 10;
  append_uint16(checksum_position, ipv4_header_checksum(ipv4_header, ipv4_header_size));

  // UDP header
  append_uint16(position, 5000); // source port
  append_uint16(position, 6000); // destination port
  append_uint16(position, static_cast<uint16_t>(udp_header_size + payload.size()));
  append_uint16(position, 0); // no checksum

  append_bytes(position, payload.data(), payload.size());

  return static_cast<size_t>(position - packet);
}

int main()
{
  quill::Backend::start();

  quill::RotatingFileSinkConfig sink_config;
  sink_config.set_open_mode('w');

  // Start a new capture file every hour, and name the files of the earlier hours after the time
  // of their first packet, e.g. quill_capture.20260101_140000.pcap. Each file starts with the
  // pcap file header, so it is a capture that can be read on its own
  sink_config.set_rotation_frequency_and_interval('H', 1);
  sink_config.set_rotation_naming_scheme(quill::RotatingFileSinkConfig::RotationNamingScheme::DateAndTime);
  sink_config.set_timezone(quill::Timezone::GmtTime);

  // The packets are Ethernet frames, which is the default link type
  quill::PcapWriter pcap_writer{"quill_capture.pcap", sink_config};

  std::array<unsigned char, 1500> packet;

  for (uint16_t sequence = 0; sequence < 10; ++sequence)
  {
    std::string const payload = "market data message " + std::to_string(sequence);
    size_t const packet_size = make_udp_packet(packet.data(), sequence, payload);

    // When the packet was received, in nanoseconds since epoch. A network card with hardware
    // timestamps provides it with the packet
    auto const capture_time_ns = static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count());

    // The record header and the packet are copied to the queue here, and the backend thread
    // writes them to the file
    pcap_writer.write_packet(packet.data(), packet_size, capture_time_ns);
  }

  // Writes the packets that are still queued and closes the file
  pcap_writer.close();

  quill::Backend::stop();

  std::cout << "Wrote 10 packets to quill_capture.pcap\n";
  return 0;
}
