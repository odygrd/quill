// BinaryDataFuzzer - Stress tests binary data logging with BinaryDataDeferredFormatCodec
//
// This fuzzer tests a completely different code path from text logging:
// - Binary data serialization (size header + raw bytes)
// - Variable-sized binary messages (0 bytes - 1MB)
// - Edge cases: null data, zero size, max uint32_t size
// - Special bytes: 0x00, 0xFF, newlines, control characters
// - Binary file writing (no text formatting, raw output)
// - memcpy operations and buffer boundaries
//
// This is critical for users logging binary protocols (PCAP, trading, IoT, etc.)

#define FUZZER_LOG_FILENAME "binary_data_fuzz.bin"
#define FUZZER_USE_BINARY_MODE 1           // Enable binary file mode

#include "FuzzerHelper.h"

#include "quill/BinaryDataDeferredFormatCodec.h"
#include "quill/LogMacros.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// Binary protocol tag types for different protocols
struct PcapProtocol
{
};
struct WeatherProtocol
{
};
struct IoTProtocol
{
};
struct GenericBinaryProtocol
{
};

// Type aliases for different binary data types
using PcapData = quill::BinaryData<PcapProtocol>;
using WeatherData = quill::BinaryData<WeatherProtocol>;
using IoTData = quill::BinaryData<IoTProtocol>;
using GenericBinaryData = quill::BinaryData<GenericBinaryProtocol>;

// Formatter for PCAP data (just write raw binary)
template <>
struct fmtquill::formatter<PcapData>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::PcapData const& bin_data, format_context& ctx) const
  {
    auto out = ctx.out();
    // Write size header
    auto size = bin_data.size();
    char const* size_ptr = reinterpret_cast<char const*>(&size);
    out = std::copy(size_ptr, size_ptr + sizeof(size), out);
    // Write raw data
    char const* data_ptr = reinterpret_cast<char const*>(bin_data.data());
    out = std::copy(data_ptr, data_ptr + size, out);
    return out;
  }
};

// Formatter for Weather data
template <>
struct fmtquill::formatter<WeatherData>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::WeatherData const& bin_data, format_context& ctx) const
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

// Formatter for IoT data
template <>
struct fmtquill::formatter<IoTData>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::IoTData const& bin_data, format_context& ctx) const
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

// Formatter for generic binary
template <>
struct fmtquill::formatter<GenericBinaryData>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::GenericBinaryData const& bin_data, format_context& ctx) const
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

// Codec specializations
template <>
struct quill::Codec<PcapData> : quill::BinaryDataDeferredFormatCodec<PcapData>
{
};

template <>
struct quill::Codec<WeatherData> : quill::BinaryDataDeferredFormatCodec<WeatherData>
{
};

template <>
struct quill::Codec<IoTData> : quill::BinaryDataDeferredFormatCodec<IoTData>
{
};

template <>
struct quill::Codec<GenericBinaryData> : quill::BinaryDataDeferredFormatCodec<GenericBinaryData>
{
};

class FuzzDataExtractor
{
public:
  explicit FuzzDataExtractor(uint8_t const* data, size_t size)
    : _data(data), _size(size), _offset(0)
  {
  }

  bool has_data() const { return _offset < _size; }

  uint8_t get_byte()
  {
    if (_offset < _size)
      return _data[_offset++];
    return 0;
  }

  uint32_t get_uint32()
  {
    uint32_t value = 0;
    if (_offset + sizeof(uint32_t) <= _size)
    {
      std::memcpy(&value, _data + _offset, sizeof(uint32_t));
      _offset += sizeof(uint32_t);
    }
    return value;
  }

  // Get raw bytes as std::byte pointer (for BinaryData)
  std::byte const* get_binary_data(size_t length)
  {
    if (_offset >= _size || length == 0)
      return nullptr;

    // Clamp to remaining data
    size_t available = _size - _offset;
    size_t actual_len = (length < available) ? length : available;

    std::byte const* result = reinterpret_cast<std::byte const*>(_data + _offset);
    _offset += actual_len;
    return result;
  }

  size_t remaining() const { return _size - _offset; }

private:
  uint8_t const* _data;
  size_t _size;
  size_t _offset;
};

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size)
{
  if (size < 3 || !g_logger)
    return 0;

  FuzzDataExtractor extractor(data, size);

  // First byte: choose strategy
  uint8_t strategy = extractor.get_byte();

  switch (strategy % 16)
  {
  case 0:
  {
    // Strategy 0: Single large binary message using all remaining input
    if (extractor.remaining() > 0)
    {
      size_t msg_size = extractor.remaining();
      std::byte const* binary_data = extractor.get_binary_data(msg_size);
      LOG_INFO(g_logger, "{}", GenericBinaryData{binary_data, msg_size});
    }
    break;
  }

  case 1:
  {
    // Strategy 1: Variable-sized binary messages
    // Tests: different size encodings, queue wraparound with binary data
    while (extractor.remaining() > 1)
    {
      uint8_t size_selector = extractor.get_byte();
      size_t msg_size = 0;

      switch (size_selector % 8)
      {
      case 0:
        msg_size = 0;
        break; // Empty
      case 1:
        msg_size = 1;
        break; // Tiny
      case 2:
        msg_size = 64;
        break; // Small
      case 3:
        msg_size = 256;
        break; // Medium
      case 4:
        msg_size = 1024;
        break; // Large
      case 5:
        msg_size = 4096;
        break; // Very large
      case 6:
        msg_size = 65536;
        break; // Huge (64KB)
      case 7:
        msg_size = extractor.remaining();
        break; // Use all
      }

      if (msg_size <= extractor.remaining())
      {
        std::byte const* binary_data = extractor.get_binary_data(msg_size);
        LOG_INFO(g_logger, "{}", PcapData{binary_data, msg_size});
      }
    }
    break;
  }

  case 2:
  {
    // Strategy 2: Null data and edge cases
    // Tests: nullptr handling, zero size
    std::byte const* null_ptr = nullptr;
    LOG_INFO(g_logger, "{}", GenericBinaryData{null_ptr, 0});

    if (extractor.has_data())
    {
      uint8_t byte = extractor.get_byte();
      std::byte const* ptr = reinterpret_cast<std::byte const*>(&byte);
      LOG_INFO(g_logger, "{}", GenericBinaryData{ptr, 0}); // Non-null but zero size
    }
    break;
  }

  case 3:
  {
    // Strategy 3: Messages with special bytes
    // Tests: 0x00, 0xFF, newlines (\n, \r), control characters
    if (extractor.remaining() >= 16)
    {
      std::array<uint8_t, 16> special_buffer{};

      // Fill with special bytes from fuzzer input
      for (size_t i = 0; i < special_buffer.size() && extractor.has_data(); ++i)
      {
        special_buffer[i] = extractor.get_byte();
      }

      LOG_INFO(g_logger, "{}",
               WeatherData{reinterpret_cast<std::byte const*>(special_buffer.data()), special_buffer.size()});
    }
    break;
  }

  case 4:
  {
    // Strategy 4: All zeros
    // Tests: memcpy of uniform data
    size_t zero_size = extractor.get_byte() % 128 + 1;
    std::vector<uint8_t> zeros(zero_size, 0x00);
    LOG_INFO(g_logger, "{}", IoTData{reinterpret_cast<std::byte const*>(zeros.data()), zeros.size()});
    break;
  }

  case 5:
  {
    // Strategy 5: All 0xFF
    // Tests: memcpy of all-ones data
    size_t ff_size = extractor.get_byte() % 128 + 1;
    std::vector<uint8_t> ffs(ff_size, 0xFF);
    LOG_INFO(g_logger, "{}", IoTData{reinterpret_cast<std::byte const*>(ffs.data()), ffs.size()});
    break;
  }

  case 6:
  {
    // Strategy 6: Messages with embedded newlines
    // Tests: newline preservation in binary mode
    if (extractor.remaining() >= 8)
    {
      std::array<uint8_t, 8> buffer{};
      for (size_t i = 0; i < buffer.size(); ++i)
      {
        if (i % 2 == 0)
          buffer[i] = '\n'; // Newline
        else
          buffer[i] = extractor.get_byte();
      }
      LOG_INFO(g_logger, "{}", PcapData{reinterpret_cast<std::byte const*>(buffer.data()), buffer.size()});
    }
    break;
  }

  case 7:
  {
    // Strategy 7: Size boundary testing
    // Tests: sizes around uint32_t boundaries, power-of-2 sizes
    size_t boundary_sizes[] = {
      0,     1,     2,    3, 4, // Very small
      127,   128,   129,        // Byte boundary
      255,   256,   257,        // Byte boundary
      1023,  1024,  1025,       // 1KB boundary
      4095,  4096,  4097,       // 4KB boundary
      65535, 65536, 65537       // 64KB boundary
    };

    uint8_t size_idx = extractor.get_byte() % (sizeof(boundary_sizes) / sizeof(boundary_sizes[0]));
    size_t target_size = boundary_sizes[size_idx];

    if (target_size <= extractor.remaining())
    {
      std::byte const* binary_data = extractor.get_binary_data(target_size);
      LOG_INFO(g_logger, "{}", GenericBinaryData{binary_data, target_size});
    }
    break;
  }

  case 8:
  {
    // Strategy 8: Burst of small binary messages
    // Tests: rapid binary message encoding
    uint32_t burst_count = extractor.get_uint32() % 50;
    for (uint32_t i = 0; i < burst_count && extractor.remaining() > 4; ++i)
    {
      size_t msg_size = extractor.get_byte() % 32 + 1;
      if (msg_size <= extractor.remaining())
      {
        std::byte const* binary_data = extractor.get_binary_data(msg_size);
        LOG_INFO(g_logger, "{}", WeatherData{binary_data, msg_size});
      }
    }
    break;
  }

  case 9:
  {
    // Strategy 9: Alternating protocol types
    // Tests: different BinaryData<T> types in same queue
    uint8_t count = extractor.get_byte() % 10;
    for (uint8_t i = 0; i < count && extractor.remaining() > 4; ++i)
    {
      size_t msg_size = extractor.get_byte() % 64 + 1;
      if (msg_size > extractor.remaining())
        msg_size = extractor.remaining();

      std::byte const* binary_data = extractor.get_binary_data(msg_size);

      switch (i % 4)
      {
      case 0:
        LOG_INFO(g_logger, "{}", PcapData{binary_data, msg_size});
        break;
      case 1:
        LOG_INFO(g_logger, "{}", WeatherData{binary_data, msg_size});
        break;
      case 2:
        LOG_INFO(g_logger, "{}", IoTData{binary_data, msg_size});
        break;
      case 3:
        LOG_INFO(g_logger, "{}", GenericBinaryData{binary_data, msg_size});
        break;
      }
    }
    break;
  }

  case 10:
  {
    // Strategy 10: Very large binary messages (256KB - 1MB)
    // Tests: large allocations, queue expansion with binary data
    size_t large_sizes[] = {262144, 524288, 1048576}; // 256KB, 512KB, 1MB
    uint8_t size_idx = extractor.get_byte() % (sizeof(large_sizes) / sizeof(large_sizes[0]));
    size_t target_size = large_sizes[size_idx];

    // Create large binary buffer from fuzzer input (repeating pattern)
    std::vector<uint8_t> large_buffer;
    large_buffer.reserve(target_size);

    while (large_buffer.size() < target_size && extractor.has_data())
    {
      uint8_t byte = extractor.get_byte();
      size_t repeat_count = std::min(size_t(1024), target_size - large_buffer.size());
      large_buffer.insert(large_buffer.end(), repeat_count, byte);
    }

    if (!large_buffer.empty())
    {
      LOG_INFO(
        g_logger, "{}",
        GenericBinaryData{reinterpret_cast<std::byte const*>(large_buffer.data()), large_buffer.size()});
    }
    break;
  }

  case 11:
  {
    // Strategy 11: Mixed with regular log messages
    // Tests: binary and text logging in same queue
    if (extractor.remaining() > 8)
    {
      uint32_t val = extractor.get_uint32();
      LOG_INFO(g_logger, "Text message: {}", val);

      size_t bin_size = extractor.get_byte() % 64;
      if (bin_size <= extractor.remaining())
      {
        std::byte const* binary_data = extractor.get_binary_data(bin_size);
        LOG_INFO(g_logger, "{}", PcapData{binary_data, bin_size});
      }

      LOG_INFO(g_logger, "Another text: {}", val * 2);
    }
    break;
  }

  case 12:
  {
    // Strategy 12: Repeating patterns
    // Tests: compressibility, pattern detection bugs
    if (extractor.has_data())
    {
      uint8_t pattern = extractor.get_byte();
      size_t repeat_count = extractor.get_byte() % 128 + 1;
      std::vector<uint8_t> pattern_buffer(repeat_count, pattern);
      LOG_INFO(g_logger, "{}",
               IoTData{reinterpret_cast<std::byte const*>(pattern_buffer.data()), pattern_buffer.size()});
    }
    break;
  }

  case 13:
  {
    // Strategy 13: Incrementing sequence
    // Tests: non-random data patterns
    size_t seq_size = extractor.get_byte() % 128 + 1;
    std::vector<uint8_t> seq_buffer(seq_size);
    for (size_t i = 0; i < seq_size; ++i)
    {
      seq_buffer[i] = static_cast<uint8_t>(i);
    }
    LOG_INFO(g_logger, "{}",
             WeatherData{reinterpret_cast<std::byte const*>(seq_buffer.data()), seq_buffer.size()});
    break;
  }

  case 14:
  {
    // Strategy 14: Aligned vs unaligned data
    // Tests: alignment handling in memcpy
    if (extractor.remaining() >= 32)
    {
      // Aligned: standard array
      std::array<uint32_t, 4> aligned{};
      for (size_t i = 0; i < 4 && extractor.remaining() >= 4; ++i)
      {
        aligned[i] = extractor.get_uint32();
      }
      LOG_INFO(g_logger, "{}",
               GenericBinaryData{reinterpret_cast<std::byte const*>(aligned.data()), sizeof(aligned)});

      // Unaligned: offset by 1 byte
      if (extractor.remaining() >= 17)
      {
        std::array<uint8_t, 17> unaligned_buffer{};
        for (size_t i = 0; i < 17 && extractor.has_data(); ++i)
        {
          unaligned_buffer[i] = extractor.get_byte();
        }
        // Log from offset 1 (unaligned)
        LOG_INFO(g_logger, "{}",
                 GenericBinaryData{reinterpret_cast<std::byte const*>(&unaligned_buffer[1]), 16});
      }
    }
    break;
  }

  case 15:
  {
    // Strategy 15: Random size from fuzzer input
    // Tests: truly random sizes driven by fuzzer
    if (extractor.remaining() >= 4)
    {
      uint32_t random_size = extractor.get_uint32();

      // Clamp to reasonable size (1MB max) and available data
      random_size = random_size % 1048576; // Max 1MB
      if (random_size > extractor.remaining())
        random_size = static_cast<uint32_t>(extractor.remaining());

      if (random_size > 0)
      {
        std::byte const* binary_data = extractor.get_binary_data(random_size);
        LOG_INFO(g_logger, "{}", GenericBinaryData{binary_data, random_size});
      }
    }
    break;
  }
  }

  return 0;
}

