#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "quill/Backend.h"
#include "quill/PcapWriter.h"
#include "quill/sinks/FileSink.h"

static constexpr size_t total_iterations = 1'000'000;

/** The sizes of the captured frames: a short message, a typical one and one that fills the MTU */
static constexpr size_t frame_sizes[] = {96, 512, 1400};

/**
 * Measures how much network traffic PcapWriter writes to a capture file. The backend worker just
 * spins, so we just measure the total time elapsed for total_iterations
 */
int main()
{
  // main thread affinity - pinning can legitimately fail (e.g. Apple Silicon does not support
  // the affinity policy); warn and continue instead of terminating the benchmark
#if defined(QUILL_NO_EXCEPTIONS)
  quill::detail::set_cpu_affinity({1});
#else
  try
  {
    quill::detail::set_cpu_affinity({1});
  }
  catch (std::exception const& e)
  {
    std::cerr << "Failed to set cpu affinity: " << e.what() << std::endl;
  }
#endif

  quill::BackendOptions backend_options;
  backend_options.cpu_affinity = {5};
  backend_options.sleep_duration = std::chrono::nanoseconds{0};
  backend_options.log_timestamp_ordering_grace_period = std::chrono::microseconds{1};

  // Start the logging backend thread
  quill::Backend::start(backend_options);

  quill::PcapWriter pcap_writer{"quill_backend_throughput_pcap.pcap",
                                []()
                                {
                                  quill::FileSinkConfig cfg;
                                  cfg.set_open_mode('w');
                                  return cfg;
                                }()};

  // An Ethernet frame with the EtherType for local experiments. Its payload starts with the
  // number of the packet
  std::array<unsigned char, 1400> frame{};
  frame[12] = 0x88;
  frame[13] = 0xb5;
  static constexpr size_t payload_offset = 14;

  // The timestamps of a capture come with the packets, e.g. from the network card
  uint64_t timestamp_ns = 1'700'000'000'000'000'000ull;

  pcap_writer.write_packet(frame.data(), frame_sizes[0], timestamp_ns);
  pcap_writer.flush();

  uint64_t total_bytes{0};

  // start counting the time until backend worker finishes
  auto const start_time = std::chrono::steady_clock::now();
  for (size_t iteration = 0; iteration < total_iterations; ++iteration)
  {
    // Cycle between three frame sizes: a capture does not repeat a single one
    size_t const frame_size = frame_sizes[iteration % 3];
    std::memcpy(frame.data() + payload_offset, &iteration, sizeof(iteration));
    timestamp_ns += 1000;

    pcap_writer.write_packet(frame.data(), frame_size, timestamp_ns);
    total_bytes += frame_size;
  }

  // block until all packets are flushed
  pcap_writer.flush();

  auto const end_time = std::chrono::steady_clock::now();
  auto const delta = end_time - start_time;
  auto delta_d = std::chrono::duration_cast<std::chrono::duration<double>>(delta).count();

  std::cout << fmtquill::format(
                 "Throughput is {:.2f} million packets/sec average, {:.2f} Gbit/sec of captured "
                 "traffic, total time elapsed: {} ms for {} packets of 96, 512 and 1400 bytes \n",
                 total_iterations / delta_d / 1e6, static_cast<double>(total_bytes) * 8 / delta_d / 1e9,
                 std::chrono::duration_cast<std::chrono::milliseconds>(delta).count(), total_iterations)
            << std::endl;
}
