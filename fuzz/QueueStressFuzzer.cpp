// QueueStressFuzzer - Stress tests queue under high load with variable message sizes and types
//
// This fuzzer differs from others by using a much higher flush limit (64KB vs 2KB),
// allowing more messages to queue up before the backend processes them. This stresses:
// - Queue reallocation and growth
// - Wraparound conditions with mixed message sizes
// - Memory pressure when backend lags behind frontend
// - Race conditions under sustained load
// - All type encoding paths: basic types, char const*, char arrays, custom codecs, enums

#define FUZZER_LOG_FILENAME "queue_stress_fuzz.log"
#define FUZZER_IMMEDIATE_FLUSH_LIMIT 65536 // 32x higher than default for queue stress

#include "FuzzerHelper.h"

#include "quill/DeferredFormatCodec.h"
#include "quill/DirectFormatCodec.h"
#include "quill/HelperMacros.h"
#include "quill/LogMacros.h"
#include "quill/core/Codec.h"

#include "quill/std/Array.h"
#include "quill/std/Vector.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

// Custom type with DeferredFormatCodec (trivially copyable)
struct DeferredType
{
  DeferredType() = default;
  DeferredType(int id, double value) : id(id), value(value) {}

  int id{};
  double value{};
};

template <>
struct fmtquill::formatter<DeferredType>
{
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(::DeferredType const& obj, FormatContext& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "Deferred{{id:{}, val:{}}}", obj.id, obj.value);
  }
};

template <>
struct quill::Codec<DeferredType> : quill::DeferredFormatCodec<DeferredType>
{
};

// Custom type with DirectFormatCodec
struct DirectType
{
  DirectType() = default;
  DirectType(int count, std::string name) : count(count), name(std::move(name)) {}

  int count{};
  std::string name;
};

template <>
struct fmtquill::formatter<DirectType>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::DirectType const& obj, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "Direct{{cnt:{}, name:{}}}", obj.count, obj.name);
  }
};

template <>
struct quill::Codec<DirectType> : quill::DirectFormatCodec<DirectType>
{
};

// Scoped enum
enum class Status : uint8_t
{
  Idle = 0,
  Running = 1,
  Failed = 2,
  Success = 3
};

std::ostream& operator<<(std::ostream& os, Status const& s)
{
  switch (s)
  {
  case Status::Idle:
    os << "Idle";
    break;
  case Status::Running:
    os << "Running";
    break;
  case Status::Failed:
    os << "Failed";
    break;
  case Status::Success:
    os << "Success";
    break;
  }
  return os;
}

QUILL_LOGGABLE_DIRECT_FORMAT(Status)

// Unscoped enum
enum Priority
{
  Low = 0,
  Medium = 1,
  High = 2,
  Critical = 3
};

std::ostream& operator<<(std::ostream& os, Priority const& p)
{
  switch (p)
  {
  case Low:
    os << "Low";
    break;
  case Medium:
    os << "Medium";
    break;
  case High:
    os << "High";
    break;
  case Critical:
    os << "Critical";
    break;
  }
  return os;
}

QUILL_LOGGABLE_DIRECT_FORMAT(Priority)

// Helper to extract data from fuzzer input
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

  // Extract a string of specified length from fuzzer data
  std::string get_bytes(size_t length)
  {
    if (_offset >= _size || length == 0)
      return "";

    // Clamp to remaining data
    size_t available = _size - _offset;
    size_t actual_len = (length < available) ? length : available;

    std::string result(reinterpret_cast<char const*>(_data + _offset), actual_len);
    _offset += actual_len;
    return result;
  }

  size_t remaining() const { return _size - _offset; }

  int8_t get_int8() { return static_cast<int8_t>(get_byte()); }

  int16_t get_int16()
  {
    int16_t value = 0;
    if (_offset + sizeof(int16_t) <= _size)
    {
      std::memcpy(&value, _data + _offset, sizeof(int16_t));
      _offset += sizeof(int16_t);
    }
    return value;
  }

  uint16_t get_uint16()
  {
    uint16_t value = 0;
    if (_offset + sizeof(uint16_t) <= _size)
    {
      std::memcpy(&value, _data + _offset, sizeof(uint16_t));
      _offset += sizeof(uint16_t);
    }
    return value;
  }

  int32_t get_int32() { return static_cast<int32_t>(get_uint32()); }

  int64_t get_int64()
  {
    int64_t value = 0;
    if (_offset + sizeof(int64_t) <= _size)
    {
      std::memcpy(&value, _data + _offset, sizeof(int64_t));
      _offset += sizeof(int64_t);
    }
    return value;
  }

  uint64_t get_uint64()
  {
    uint64_t value = 0;
    if (_offset + sizeof(uint64_t) <= _size)
    {
      std::memcpy(&value, _data + _offset, sizeof(uint64_t));
      _offset += sizeof(uint64_t);
    }
    return value;
  }

  float get_float()
  {
    float value = 0.0f;
    if (_offset + sizeof(float) <= _size)
    {
      std::memcpy(&value, _data + _offset, sizeof(float));
      _offset += sizeof(float);
    }
    return value;
  }

  double get_double()
  {
    double value = 0.0;
    if (_offset + sizeof(double) <= _size)
    {
      std::memcpy(&value, _data + _offset, sizeof(double));
      _offset += sizeof(double);
    }
    return value;
  }

  long double get_long_double()
  {
    long double value = 0.0L;
    if (_offset + sizeof(long double) <= _size)
    {
      std::memcpy(&value, _data + _offset, sizeof(long double));
      _offset += sizeof(long double);
    }
    return value;
  }

  bool get_bool() { return get_byte() & 1; }

  char get_char() { return static_cast<char>(get_byte()); }

  // Get null-terminated C string from fuzzer data
  // NOTE: Returns pointer to internal buffer - valid until next call
  char const* get_c_string(size_t max_len = 256)
  {
    if (_offset >= _size)
    {
      _c_string_buffer[0] = '\0';
      return _c_string_buffer.data();
    }

    size_t len = get_byte() % (max_len + 1);
    size_t actual_len = 0;

    for (size_t i = 0; i < len && _offset < _size && actual_len < _c_string_buffer.size() - 1; ++i)
    {
      _c_string_buffer[actual_len++] = static_cast<char>(_data[_offset++]);
    }

    // Ensure null termination - required by quill for char const*
    _c_string_buffer[actual_len] = '\0';
    return _c_string_buffer.data();
  }

  std::string_view get_string_view()
  {
    if (_offset >= _size)
      return "";

    size_t len = get_byte();
    if (_offset + len > _size)
      len = _size - _offset;

    std::string_view result(reinterpret_cast<char const*>(_data + _offset), len);
    _offset += len;
    return result;
  }

private:
  uint8_t const* _data;
  size_t _size;
  size_t _offset;
  std::array<char, 512> _c_string_buffer; // Buffer for null-terminated C strings
};

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size)
{
  // Need at least 10 bytes to do anything useful
  if (size < 10 || !g_logger)
    return 0;

  FuzzDataExtractor extractor(data, size);

  // Second byte: choose strategy
  uint8_t strategy = extractor.get_byte();

  switch (strategy % 24)
  {
  case 0:
  {
    // Strategy 0: Single very large message using all remaining fuzzer input
    // This stresses: large memory allocation, queue growth, single huge write
    if (extractor.remaining() > 0)
    {
      std::string large_msg = extractor.get_bytes(extractor.remaining());
      LOG_INFO(g_logger, "Large: {}", large_msg);
    }
    break;
  }

  case 1:
  {
    // Strategy 1: Multiple variable-sized messages
    // This stresses: queue wraparound, mixed allocation sizes, boundary conditions
    while (extractor.has_data() && extractor.remaining() > 2)
    {
      // Use next byte to determine message size
      uint8_t size_selector = extractor.get_byte();
      size_t msg_size = 0;

      // Create variable message sizes from tiny to large
      switch (size_selector % 8)
      {
      case 0:
        msg_size = 1;
        break; // Tiny
      case 1:
        msg_size = 16;
        break; // Small
      case 2:
        msg_size = 256;
        break; // Medium
      case 3:
        msg_size = 1024;
        break; // Large
      case 4:
        msg_size = 4096;
        break; // Very large
      case 5:
        msg_size = 16384;
        break; // Huge
      case 6:
        msg_size = 65536;
        break; // 64KB
      case 7:
        msg_size = extractor.remaining();
        break; // Use all remaining
      }

      std::string msg = extractor.get_bytes(msg_size);
      if (!msg.empty())
      {
        LOG_INFO(g_logger, "Msg: {}", msg);
      }
    }
    break;
  }

  case 2:
  {
    // Strategy 2: Rapid-fire small messages in tight loop
    // This stresses: queue throughput, rapid allocation/deallocation, backend catching up
    uint32_t count = extractor.get_uint32() % 100; // Up to 100 iterations
    for (uint32_t i = 0; i < count && extractor.has_data(); ++i)
    {
      uint8_t byte = extractor.get_byte();
      LOG_INFO(g_logger, "Rapid {}: {}", i, static_cast<int>(byte));
    }
    break;
  }

  case 3:
  {
    // Strategy 3: Alternating tiny and large messages
    // This stresses: queue wraparound with mixed sizes, fragmentation
    bool large = false;
    while (extractor.remaining() > 4)
    {
      if (large)
      {
        // Large message (use up to 8KB or remaining)
        size_t size = extractor.get_uint32() % 8192;
        if (size == 0)
          size = 1;
        std::string msg = extractor.get_bytes(size);
        LOG_INFO(g_logger, "Big: {}", msg);
      }
      else
      {
        // Tiny message
        uint8_t byte = extractor.get_byte();
        LOG_INFO(g_logger, "Tiny: {}", static_cast<int>(byte));
      }
      large = !large;
    }
    break;
  }

  case 4:
  {
    // Strategy 4: Multiple arguments with variable data
    // This stresses: encoding of multiple arguments, mixed types
    while (extractor.remaining() > 8)
    {
      uint32_t val = extractor.get_uint32();
      std::string str = extractor.get_bytes(extractor.get_byte() % 128);
      LOG_INFO(g_logger, "Multi: {} {}", val, str);
    }
    break;
  }

  case 5:
  {
    // Strategy 5: LOGV macro with variable data
    // This stresses: LOGV encoding path
    while (extractor.remaining() > 8)
    {
      uint32_t a = extractor.get_uint32();
      uint32_t b = extractor.get_uint32();
      LOGV_INFO(g_logger, "LOGV test", a, b);
    }
    break;
  }

  case 6:
  {
    // Strategy 6: Near queue-capacity sized messages
    // This stresses: messages that nearly fill the queue, wraparound edge cases
    // Initial queue capacity is 4096 bytes, try messages near that size
    size_t sizes[] = {4090, 4092, 4094, 4095, 4096, 4097, 8192, 16384};
    uint8_t size_idx = extractor.get_byte() % (sizeof(sizes) / sizeof(sizes[0]));
    size_t target_size = sizes[size_idx];

    std::string msg = extractor.get_bytes(target_size);
    if (!msg.empty())
    {
      LOG_INFO(g_logger, "Boundary: {}", msg);
    }
    break;
  }

  case 7:
  {
    // Strategy 7: Mixed format specifiers with fuzzer data
    // This stresses: format parsing, type encoding
    while (extractor.remaining() > 8)
    {
      uint32_t val = extractor.get_uint32();
      std::string str = extractor.get_bytes(extractor.get_byte() % 64);

      // Vary the format
      uint8_t format_selector = extractor.get_byte();
      switch (format_selector % 4)
      {
      case 0:
        LOG_INFO(g_logger, "Hex: {:x}, Str: {}", val, str);
        break;
      case 1:
        LOG_INFO(g_logger, "Oct: {:o}, Str: {}", val, str);
        break;
      case 2:
        LOG_INFO(g_logger, "Bin: {:b}, Str: {}", val, str);
        break;
      case 3:
        LOG_INFO(g_logger, "Dec: {}, Str: {}", val, str);
        break;
      }
    }
    break;
  }

  case 8:
  {
    // Strategy 8: Extremely large single messages (256KB - 4MB range)
    // This stresses: very large allocations, queue capacity expansion
    size_t large_sizes[] = {262144, 524288, 1048576, 2097152, 4194304}; // 256KB, 512KB, 1MB, 2MB, 4MB
    uint8_t size_idx = extractor.get_byte() % (sizeof(large_sizes) / sizeof(large_sizes[0]));
    size_t target_size = large_sizes[size_idx];

    // Create a large string with pattern from fuzzer data
    std::string large_msg;
    large_msg.reserve(target_size);

    // Fill with repeating pattern from fuzzer input
    while (large_msg.size() < target_size && extractor.has_data())
    {
      uint8_t byte = extractor.get_byte();
      // Repeat the byte multiple times
      size_t repeat_count = std::min(size_t(1024), target_size - large_msg.size());
      large_msg.append(repeat_count, static_cast<char>(byte));
    }

    if (!large_msg.empty())
    {
      LOG_INFO(g_logger, "XLarge: {}", large_msg);
    }
    break;
  }

  case 9:
  {
    // Strategy 9: Different log level macros with same data
    // This stresses: different log level encoding paths
    if (extractor.remaining() > 4)
    {
      std::string msg = extractor.get_bytes(extractor.get_byte() % 128);
      uint32_t val = extractor.get_uint32();

      uint8_t level = extractor.get_byte() % 9;
      switch (level)
      {
      case 0:
        LOG_TRACE_L3(g_logger, "L3: {} {}", val, msg);
        break;
      case 1:
        LOG_TRACE_L2(g_logger, "L2: {} {}", val, msg);
        break;
      case 2:
        LOG_TRACE_L1(g_logger, "L1: {} {}", val, msg);
        break;
      case 3:
        LOG_DEBUG(g_logger, "Debug: {} {}", val, msg);
        break;
      case 4:
        LOG_INFO(g_logger, "Info: {} {}", val, msg);
        break;
      case 5:
        LOG_NOTICE(g_logger, "Notice: {} {}", val, msg);
        break;
      case 6:
        LOG_WARNING(g_logger, "Warn: {} {}", val, msg);
        break;
      case 7:
        LOG_ERROR(g_logger, "Error: {} {}", val, msg);
        break;
      case 8:
        LOG_CRITICAL(g_logger, "Crit: {} {}", val, msg);
        break;
      }
    }
    break;
  }

  case 10:
  {
    // Strategy 10: Empty and minimal messages
    // This stresses: edge cases with minimal data
    LOG_INFO(g_logger, "");
    LOG_INFO(g_logger, "x");
    if (extractor.has_data())
    {
      char c = static_cast<char>(extractor.get_byte());
      LOG_INFO(g_logger, "{}", c);
    }
    break;
  }

  case 11:
  {
    // Strategy 11: Messages with special/control characters
    // This stresses: encoding of special characters, edge cases
    std::string special;
    while (extractor.has_data() && special.size() < 256)
    {
      special.push_back(static_cast<char>(extractor.get_byte()));
    }
    LOG_INFO(g_logger, "Special: {}", special);
    break;
  }

  case 12:
  {
    // Strategy 12: Burst of medium messages without pause
    // This stresses: sustained queue pressure, no time for backend to catch up
    uint32_t burst_count = extractor.get_uint32() % 50; // Up to 50 messages
    for (uint32_t i = 0; i < burst_count && extractor.remaining() > 100; ++i)
    {
      std::string msg = extractor.get_bytes(extractor.get_byte() % 200 + 50); // 50-250 bytes
      LOG_INFO(g_logger, "Burst {}: {}", i, msg);
    }
    break;
  }

  case 13:
  {
    // Strategy 13: Queue capacity edge cases
    // Test exact multiples and near-misses of queue capacity
    size_t edge_sizes[] = {
      2047,  2048,  2049,  // Near initial capacity (2048)
      4095,  4096,  4097,  // Power of 2 boundary
      8191,  8192,  8193,  // Next doubling
      16383, 16384, 16385, // Next doubling
      32767, 32768, 32769  // Larger boundary
    };

    uint8_t size_idx = extractor.get_byte() % (sizeof(edge_sizes) / sizeof(edge_sizes[0]));
    size_t target_size = edge_sizes[size_idx];

    std::string msg = extractor.get_bytes(target_size);
    if (!msg.empty())
    {
      LOG_INFO(g_logger, "Edge {}: {}", target_size, msg);
    }
    break;
  }

  case 14:
  {
    // Strategy 14: Interleaved different macro types
    // This stresses: mixing LOG and LOGV, different encoding paths
    while (extractor.remaining() > 12)
    {
      uint32_t a = extractor.get_uint32();
      uint32_t b = extractor.get_uint32();
      std::string s = extractor.get_bytes(extractor.get_byte() % 64);

      // Alternate between LOG and LOGV
      if (a & 1)
      {
        LOG_INFO(g_logger, "LOG: {} {} {}", a, b, s);
      }
      else
      {
        LOGV_INFO(g_logger, "LOGV", a, b, s);
      }
    }
    break;
  }

  case 15:
  {
    // Strategy 15: Messages at exact queue reallocation triggers
    // Create messages sized to trigger queue growth at specific points
    // Assuming queue doubles: 2KB -> 4KB -> 8KB -> 16KB -> 32KB -> 64KB...

    // Generate a sequence of messages that accumulate to just over capacity
    uint32_t num_msgs = extractor.get_byte() % 10 + 5; // 5-15 messages
    size_t per_msg_size = 300;                         // Each ~300 bytes

    for (uint32_t i = 0; i < num_msgs && extractor.has_data(); ++i)
    {
      std::string msg = extractor.get_bytes(per_msg_size);
      LOG_INFO(g_logger, "Seq {}: {}", i, msg);
    }
    break;
  }

  case 16:
  {
    // Strategy 16: All arithmetic types stress
    // This stresses: encoding of all basic arithmetic types
    if (extractor.remaining() > 64)
    {
      int8_t i8 = extractor.get_int8();
      uint8_t u8 = extractor.get_byte();
      int16_t i16 = extractor.get_int16();
      uint16_t u16 = extractor.get_uint16();
      int32_t i32 = extractor.get_int32();
      uint32_t u32 = extractor.get_uint32();
      int64_t i64 = extractor.get_int64();
      uint64_t u64 = extractor.get_uint64();
      float f = extractor.get_float();
      double d = extractor.get_double();
      long double ld = extractor.get_long_double();
      bool b = extractor.get_bool();
      char c = extractor.get_char();

      LOG_INFO(g_logger,
               "Arithmetic: i8={} u8={} i16={} u16={} i32={} u32={} i64={} u64={} f={} d={} ld={} "
               "b={} c={}",
               i8, u8, i16, u16, i32, u32, i64, u64, f, d, ld, b, c);
    }
    break;
  }

  case 17:
  {
    // Strategy 17: char const* (null-terminated) stress
    // This stresses: null-terminated string handling, different from std::string
    char const* cstr1 = extractor.get_c_string(128);
    char const* cstr2 = extractor.get_c_string(256);
    char const* cstr3 = extractor.get_c_string(64);

    LOG_INFO(g_logger, "C-strings: '{}' '{}' '{}'", cstr1, cstr2, cstr3);
    break;
  }

  case 18:
  {
    // Strategy 18: char arrays stress
    // This stresses: C-style array encoding
    if (extractor.remaining() > 20)
    {
      char arr1[8];
      char arr2[16];
      char arr3[32];

      for (size_t i = 0; i < sizeof(arr1) && extractor.has_data(); ++i)
        arr1[i] = extractor.get_char();
      for (size_t i = 0; i < sizeof(arr2) && extractor.has_data(); ++i)
        arr2[i] = extractor.get_char();
      for (size_t i = 0; i < sizeof(arr3) && extractor.has_data(); ++i)
        arr3[i] = extractor.get_char();

      LOG_INFO(g_logger, "CharArrays: {} {} {}", arr1, arr2, arr3);
    }
    break;
  }

  case 19:
  {
    // Strategy 19: Deferred format custom types
    // This stresses: DeferredFormatCodec encoding path
    if (extractor.remaining() > 32)
    {
      DeferredType dt1{extractor.get_int32(), extractor.get_double()};
      DeferredType dt2{extractor.get_int32(), extractor.get_double()};
      DeferredType dt3{extractor.get_int32(), extractor.get_double()};

      LOG_INFO(g_logger, "Deferred: {} {} {}", dt1, dt2, dt3);

      // Also test in arrays
      DeferredType arr[3] = {dt1, dt2, dt3};
      LOG_INFO(g_logger, "DeferredArray: {}", arr);

      // And in vectors
      std::vector<DeferredType> vec;
      vec.push_back(dt1);
      vec.push_back(dt2);
      LOG_INFO(g_logger, "DeferredVector: {}", vec);
    }
    break;
  }

  case 20:
  {
    // Strategy 20: Direct format custom types
    // This stresses: DirectFormatCodec encoding path
    if (extractor.remaining() > 16)
    {
      DirectType dt1{extractor.get_int32(), extractor.get_bytes(extractor.get_byte() % 32)};
      DirectType dt2{extractor.get_int32(), extractor.get_bytes(extractor.get_byte() % 32)};

      LOG_INFO(g_logger, "Direct: {} {}", dt1, dt2);

      // Test in vectors
      std::vector<DirectType> vec;
      vec.push_back(dt1);
      vec.push_back(dt2);
      LOG_INFO(g_logger, "DirectVector: {}", vec);
    }
    break;
  }

  case 21:
  {
    // Strategy 21: Enum types (scoped and unscoped)
    // This stresses: enum encoding via ostream operator<<
    if (extractor.remaining() > 8)
    {
      Status s1 = static_cast<Status>(extractor.get_byte() % 4);
      Status s2 = static_cast<Status>(extractor.get_byte() % 4);
      Status s3 = static_cast<Status>(extractor.get_byte() % 4);
      Priority p1 = static_cast<Priority>(extractor.get_byte() % 4);
      Priority p2 = static_cast<Priority>(extractor.get_byte() % 4);
      Priority p3 = static_cast<Priority>(extractor.get_byte() % 4);

      // Test various combinations
      LOG_INFO(g_logger, "ScopedEnums: {} {} {}", s1, s2, s3);
      LOG_INFO(g_logger, "UnscopedEnums: {} {} {}", p1, p2, p3);
      LOG_INFO(g_logger, "MixedEnums: status={} priority={}", s1, p1);
      LOG_INFO(g_logger, "AllEnums: {} {} {} {} {} {}", s1, s2, s3, p1, p2, p3);
    }
    break;
  }

  case 22:
  {
    // Strategy 22: Mixed types in one log statement
    // This stresses: encoding of different type paths in single message
    if (extractor.remaining() > 64)
    {
      int32_t i = extractor.get_int32();
      std::string s = extractor.get_bytes(extractor.get_byte() % 64);
      char const* cs = extractor.get_c_string(32);
      double d = extractor.get_double();
      DeferredType dt{extractor.get_int32(), extractor.get_double()};
      DirectType direct{extractor.get_int32(), extractor.get_bytes(extractor.get_byte() % 16)};
      Status status = static_cast<Status>(extractor.get_byte() % 4);
      bool b = extractor.get_bool();
      std::string_view sv = extractor.get_string_view();

      LOG_INFO(g_logger, "Mixed: i={} s='{}' cs='{}' d={} dt={} direct={} status={} b={} sv='{}'",
               i, s, cs, d, dt, direct, status, b, sv);
    }
    break;
  }

  case 23:
  {
    // Strategy 23: String types variations
    // This stresses: std::string vs char const* vs std::string_view encoding
    if (extractor.remaining() > 16)
    {
      std::string str = extractor.get_bytes(extractor.get_byte() % 128);
      char const* cstr = extractor.get_c_string(128);
      std::string_view sv = extractor.get_string_view();

      // Log them separately
      LOG_INFO(g_logger, "String: {}", str);
      LOG_INFO(g_logger, "CStr: {}", cstr);
      LOG_INFO(g_logger, "StringView: {}", sv);

      // Log them together
      LOG_INFO(g_logger, "AllStrings: str='{}' cstr='{}' sv='{}'", str, cstr, sv);

      // Mix with other types
      int32_t val = extractor.get_int32();
      LOG_INFO(g_logger, "StringsMixed: {} '{}' '{}' '{}'", val, str, cstr, sv);
    }
    break;
  }
  }

  return 0;
}
