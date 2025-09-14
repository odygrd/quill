#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/BinaryDataDeferredFormatCodec.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <array>
#include <cstring>
#include <fstream>
#include <vector>

using namespace quill;

// Variable-sized test data structures
struct SmallData
{
  uint32_t id;
  uint16_t value;
};

struct MediumData
{
  uint32_t id;
  uint32_t value;
  bool flag;
};

struct LargeData
{
  uint32_t id;
  uint64_t timestamp;
  uint32_t value;
  char name[16];
};

// Binary data protocol
struct TestProtocol
{
};

using TestBinaryData = quill::BinaryData<TestProtocol>;

template <>
struct fmtquill::formatter<TestBinaryData>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::TestBinaryData const& bin_data, format_context& ctx) const
  {
    auto out = ctx.out();
    auto size = bin_data.size();
    char const* size_ptr = reinterpret_cast<char const*>(&size);
    out = std::copy(size_ptr, size_ptr + sizeof(size), out);
    char const* data_ptr = reinterpret_cast<char const*>(bin_data.data());
    out = std::copy(data_ptr, data_ptr + size, out);
    return out;
  }
};

template <>
struct quill::Codec<TestBinaryData> : quill::BinaryDataDeferredFormatCodec<TestBinaryData>
{
};

/**
 * Test binary file writer functionality cross-platform
 */
TEST_CASE("binary_file_writer")
{
  static constexpr size_t number_of_messages = 100;
  static constexpr char const* filename = "binary_file_writer_test.bin";
  static constexpr char const* logger_name = "binary_logger";

  BackendOptions backend_options;
  backend_options.check_printable_char = {}; // Disable printable char check for binary data
  Backend::start(backend_options);

  // Configure binary pattern formatter - no suffix for raw binary data
  PatternFormatterOptions binary_pfo;
  binary_pfo.format_pattern = "%(message)";
  binary_pfo.add_metadata_to_multi_line_logs = false;
  binary_pfo.pattern_suffix = PatternFormatterOptions::NO_SUFFIX;

  // Set up binary file sink
  auto file_sink = Frontend::create_or_get_sink<FileSink>(filename,
                                                          []()
                                                          {
                                                            FileSinkConfig cfg;
                                                            cfg.set_open_mode("wb");
                                                            return cfg;
                                                          }());

  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink), binary_pfo);

  // === PREPARE TEST DATA ===
  std::vector<std::pair<uint32_t, uint32_t>> expected_messages; // {message_type, size}
  std::vector<std::vector<uint8_t>> message_data;               // Pre-prepared message data

  for (uint32_t i = 0; i < number_of_messages; ++i)
  {
    uint32_t message_type = i % 3; // Rotate between 3 message types
    std::vector<uint8_t> data_buffer;

    if (message_type == 0)
    {
      // Small message (6 bytes)
      SmallData data;
      data.id = 100 + i;
      data.value = static_cast<uint16_t>(i);

      data_buffer.resize(sizeof(SmallData));
      std::memcpy(data_buffer.data(), &data, sizeof(SmallData));
      expected_messages.push_back({message_type, static_cast<uint32_t>(sizeof(SmallData))});
    }
    else if (message_type == 1)
    {
      // Medium message (9 bytes)
      MediumData data;
      data.id = 200 + i;
      data.value = i * 10;
      data.flag = (i % 2 == 0);

      data_buffer.resize(sizeof(MediumData));
      std::memcpy(data_buffer.data(), &data, sizeof(MediumData));
      expected_messages.push_back({message_type, static_cast<uint32_t>(sizeof(MediumData))});
    }
    else // message_type == 2
    {
      // Large message (32 bytes)
      LargeData data;
      data.id = 300 + i;
      data.timestamp = static_cast<uint64_t>(i) * 1000;
      data.value = i * 100;
      std::memset(data.name, 0, sizeof(data.name));
      std::string name = "Msg" + std::to_string(i);
      std::memcpy(data.name, name.c_str(), std::min(name.size(), sizeof(data.name) - 1));

      data_buffer.resize(sizeof(LargeData));
      std::memcpy(data_buffer.data(), &data, sizeof(LargeData));
      expected_messages.push_back({message_type, static_cast<uint32_t>(sizeof(LargeData))});
    }

    message_data.push_back(data_buffer);
  }

  // === LOG ALL PREPARED DATA ===
  for (uint32_t i = 0; i < number_of_messages; ++i)
  {
    LOG_INFO(logger, "{}", TestBinaryData{message_data[i].data(), message_data[i].size()});
  }

  // Test edge cases: binary data with newline characters in different positions
  // Case 1: Only last byte is newline
  char const edge_buffer1[5] = {'\x0b', '\x0c', '\x0d', '\x0e', '\x0a'};
  LOG_INFO(logger, "{}", TestBinaryData{reinterpret_cast<std::byte const*>(edge_buffer1), sizeof(edge_buffer1)});

  // Case 2: Only prelast byte is newline
  char const edge_buffer2[5] = {'\x0b', '\x0c', '\x0d', '\x0a', '\x0f'};
  LOG_INFO(logger, "{}", TestBinaryData{reinterpret_cast<std::byte const*>(edge_buffer2), sizeof(edge_buffer2)});

  // Case 3: Both prelast and last bytes are newlines
  char const edge_buffer3[5] = {'\x0b', '\x0c', '\x0d', '\x0a', '\x0a'};
  LOG_INFO(logger, "{}", TestBinaryData{reinterpret_cast<std::byte const*>(edge_buffer3), sizeof(edge_buffer3)});

  // Case 4: Neither prelast nor last byte is newline (control case)
  char const edge_buffer4[5] = {'\x0b', '\x0c', '\x0d', '\x0e', '\x0f'};
  LOG_INFO(logger, "{}", TestBinaryData{reinterpret_cast<std::byte const*>(edge_buffer4), sizeof(edge_buffer4)});

  logger->flush_log();
  Frontend::remove_logger(logger);
  Backend::stop();

  // === BINARY FILE VERIFICATION ===
  // Expected file format: [size_header][data][size_header][data]...
  // Where size_header is 4-byte uint32_t indicating the data size that follows
  std::ifstream infile(filename, std::ios::binary);
  REQUIRE(infile.is_open());

  // FILE SIZE VERIFICATION: Check total file size matches expectations
  infile.seekg(0, std::ios::end);
  std::streampos file_size = infile.tellg();
  infile.seekg(0, std::ios::beg);

  // Expected file size calculation:
  size_t expected_variable_size = 0;
  for (auto const& msg : expected_messages)
  {
    expected_variable_size += sizeof(uint32_t) + static_cast<size_t>(msg.second); // header + data
  }

  std::streampos expected_file_size = static_cast<std::streampos>(expected_variable_size) +
    static_cast<std::streampos>(4 * (sizeof(uint32_t) + 5));

  REQUIRE_EQ(file_size, expected_file_size);

  // === Verify variable-sized messages ===
  size_t count = 0;
  while (count < number_of_messages)
  {
    // HEADER: Read 4-byte size header for this message
    uint32_t size;
    std::array<char, sizeof(uint32_t)> size_bytes{};
    if (!infile.read(size_bytes.data(), sizeof(size_bytes)))
    {
      break;
    }
    std::memcpy(&size, size_bytes.data(), sizeof(size));

    // HEADER VERIFICATION: Size should match expected message size
    REQUIRE_EQ(size, expected_messages[count].second);

    uint32_t message_type = expected_messages[count].first;
    std::vector<char> read_buffer(size);

    // DATA: Read the actual message bytes
    REQUIRE(infile.read(read_buffer.data(), static_cast<std::streamsize>(size)));

    // DATA VERIFICATION: Verify message content based on type
    if (message_type == 0)
    {
      // Small message verification
      SmallData read_data;
      std::memcpy(&read_data, read_buffer.data(), sizeof(SmallData));

      REQUIRE_EQ(read_data.id, 100 + count);
      REQUIRE_EQ(read_data.value, static_cast<uint16_t>(count));
    }
    else if (message_type == 1)
    {
      // Medium message verification
      MediumData read_data;
      std::memcpy(&read_data, read_buffer.data(), sizeof(MediumData));

      REQUIRE_EQ(read_data.id, 200 + count);
      REQUIRE_EQ(read_data.value, count * 10);
      REQUIRE_EQ(read_data.flag, (count % 2 == 0));
    }
    else // message_type == 2
    {
      // Large message verification
      LargeData read_data;
      std::memcpy(&read_data, read_buffer.data(), sizeof(LargeData));

      REQUIRE_EQ(read_data.id, 300 + count);
      REQUIRE_EQ(read_data.timestamp, static_cast<uint64_t>(count) * 1000);
      REQUIRE_EQ(read_data.value, count * 100);

      std::string expected_name = "Msg" + std::to_string(count);
      std::string actual_name(read_data.name);

      REQUIRE_EQ(actual_name, expected_name);
    }

    count++;
  }

  // === Verify edge case messages (raw binary data with newlines) ===
  char const expected_edge_buffers[4][5] = {
    {'\x0b', '\x0c', '\x0d', '\x0e', '\x0a'}, // Case 1: Only last byte is newline
    {'\x0b', '\x0c', '\x0d', '\x0a', '\x0f'}, // Case 2: Only prelast byte is newline
    {'\x0b', '\x0c', '\x0d', '\x0a', '\x0a'}, // Case 3: Both prelast and last bytes are newlines
    {'\x0b', '\x0c', '\x0d', '\x0e', '\x0f'}  // Case 4: Neither prelast nor last byte is newline
  };

  for (int case_num = 0; case_num < 4; ++case_num)
  {
    // HEADER: Read 4-byte size header for edge case message
    uint32_t edge_size;
    std::array<char, sizeof(uint32_t)> edge_size_bytes{};
    REQUIRE(infile.read(edge_size_bytes.data(), sizeof(edge_size_bytes)));
    std::memcpy(&edge_size, edge_size_bytes.data(), sizeof(edge_size));

    // HEADER VERIFICATION: Size should be exactly 5 bytes
    REQUIRE_EQ(edge_size, 5u);

    // DATA: Read the 5 bytes of raw binary data
    char edge_read_buffer[5];
    REQUIRE(infile.read(edge_read_buffer, 5));

    // DATA VERIFICATION: Verify each byte matches exactly (including newlines)
    // This is critical - newlines in binary data must be preserved as-is
    for (int i = 0; i < 5; ++i)
    {
      REQUIRE_EQ(static_cast<unsigned char>(edge_read_buffer[i]),
                 static_cast<unsigned char>(expected_edge_buffers[case_num][i]));
    }
  }

  infile.close();
  REQUIRE_EQ(count, number_of_messages);

  testing::remove_file(filename);
}
