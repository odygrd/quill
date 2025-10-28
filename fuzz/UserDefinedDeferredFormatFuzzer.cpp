#define FUZZER_LOG_FILENAME "user_defined_deferred_fuzz.log"
#include "FuzzerHelper.h"

#include "quill/DeferredFormatCodec.h"
#include "quill/LogMacros.h"
#include "quill/core/Codec.h"

#include "quill/std/Array.h"
#include "quill/std/Deque.h"
#include "quill/std/List.h"
#include "quill/std/Map.h"
#include "quill/std/Pair.h"
#include "quill/std/Set.h"
#include "quill/std/UnorderedSet.h"
#include "quill/std/Vector.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_set>
#include <vector>

// Trivially copyable custom type with default constructor
struct TriviallyCopyableType
{
  TriviallyCopyableType() = default;
  TriviallyCopyableType(int id, double score, uint32_t count) : id(id), score(score), count(count)
  {
  }

  int id{};
  double score{};
  uint32_t count{};
};

template <>
struct fmtquill::formatter<TriviallyCopyableType>
{
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(::TriviallyCopyableType const& obj, FormatContext& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "TC{{id: {}, score: {}, count: {}}}", obj.id, obj.score,
                               obj.count);
  }
};

template <>
struct quill::Codec<TriviallyCopyableType> : quill::DeferredFormatCodec<TriviallyCopyableType>
{
};

static_assert(quill::DeferredFormatCodec<TriviallyCopyableType>::use_memcpy,
              "TriviallyCopyableType must use memcpy");

// Trivially copyable without default constructor
struct TriviallyCopyableNoDefault
{
  TriviallyCopyableNoDefault(int x, int y) : x(x), y(y) {}

  int x;
  int y;
};

template <>
struct fmtquill::formatter<TriviallyCopyableNoDefault>
{
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(::TriviallyCopyableNoDefault const& obj, FormatContext& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "TCND{{x: {}, y: {}}}", obj.x, obj.y);
  }
};

template <>
struct quill::Codec<TriviallyCopyableNoDefault> : quill::DeferredFormatCodec<TriviallyCopyableNoDefault>
{
};

static_assert(!quill::DeferredFormatCodec<TriviallyCopyableNoDefault>::use_memcpy,
              "TriviallyCopyableNoDefault should not use memcpy");

// Complex type with std::string and std::vector
class ComplexType
{
public:
  ComplexType() = default;

  ComplexType(std::string name, int value) : name(std::move(name)), value(value)
  {
    tags.push_back("tag1");
    tags.push_back("tag2");
  }

  bool operator<(ComplexType const& other) const { return this->value < other.value; }

  std::string name;
  int value{};
  std::vector<std::string> tags;
};

template <>
struct fmtquill::formatter<ComplexType>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::ComplexType const& obj, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "Complex{{name: {}, value: {}, tags: {}}}", obj.name,
                               obj.value, obj.tags);
  }
};

template <>
struct quill::Codec<ComplexType> : quill::DeferredFormatCodec<ComplexType>
{
};

// Large type (1KB) - stresses memcpy and buffer allocation
struct LargeType
{
  LargeType() = default;
  LargeType(int id, uint32_t fill_val) : id(id)
  {
    std::fill(std::begin(large_buffer), std::end(large_buffer), static_cast<uint8_t>(fill_val));
  }

  int id{};
  uint8_t large_buffer[1020]{}; // 1KB total with id
};

template <>
struct fmtquill::formatter<LargeType>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::LargeType const& obj, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "LargeType{{id: {}, buf_size: {}}}", obj.id, sizeof(obj.large_buffer));
  }
};

template <>
struct quill::Codec<LargeType> : quill::DeferredFormatCodec<LargeType>
{
};

// Empty type
struct EmptyType
{
  EmptyType() = default;
};

template <>
struct fmtquill::formatter<EmptyType>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::EmptyType const&, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "Empty{{}}");
  }
};

template <>
struct quill::Codec<EmptyType> : quill::DeferredFormatCodec<EmptyType>
{
};

// Type with padding/alignment issues
struct UnalignedType
{
  UnalignedType() = default;
  UnalignedType(uint8_t a, uint64_t b, uint8_t c, uint32_t d) : a(a), b(b), c(c), d(d) {}

  uint8_t a{};  // 1 byte + 7 padding
  uint64_t b{}; // 8 bytes
  uint8_t c{};  // 1 byte + 3 padding
  uint32_t d{}; // 4 bytes
  // Total: 24 bytes with padding
};

template <>
struct fmtquill::formatter<UnalignedType>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::UnalignedType const& obj, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "Unaligned{{a:{}, b:{}, c:{}, d:{}}}", obj.a, obj.b,
                               obj.c, obj.d);
  }
};

template <>
struct quill::Codec<UnalignedType> : quill::DeferredFormatCodec<UnalignedType>
{
};

// Type with very large string member
struct VeryLargeStringType
{
  VeryLargeStringType() = default;
  VeryLargeStringType(std::string large_str, int id) : large_str(std::move(large_str)), id(id) {}

  std::string large_str; // Can be 1MB+
  int id{};
};

template <>
struct fmtquill::formatter<VeryLargeStringType>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::VeryLargeStringType const& obj, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "VeryLarge{{id:{}, str_len:{}}}", obj.id, obj.large_str.size());
  }
};

template <>
struct quill::Codec<VeryLargeStringType> : quill::DeferredFormatCodec<VeryLargeStringType>
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

  int32_t get_int32() { return static_cast<int32_t>(get_uint32()); }

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

  std::string get_string(size_t max_len = 256)
  {
    if (_offset >= _size)
      return "";

    size_t len = get_byte() % (max_len + 1);
    std::string result;
    for (size_t i = 0; i < len && _offset < _size; ++i)
    {
      result.push_back(static_cast<char>(_data[_offset++]));
    }
    return result;
  }

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
  uint8_t selector = extractor.get_byte();

  switch (selector % 24)
  {
  case 0:
  {
    // Test trivially copyable type
    TriviallyCopyableType obj{extractor.get_int32(), extractor.get_double(), extractor.get_uint32()};
    LOG_INFO(g_logger, "TC: {}", obj);
    break;
  }
  case 1:
  {
    // Test trivially copyable no default
    TriviallyCopyableNoDefault obj{extractor.get_int32(), extractor.get_int32()};
    LOG_INFO(g_logger, "TCND: {}", obj);
    break;
  }
  case 2:
  {
    // Test complex type
    ComplexType obj{extractor.get_string(32), extractor.get_int32()};
    LOG_INFO(g_logger, "Complex: {}", obj);
    break;
  }
  case 3:
  {
    // Test array of trivially copyable
    TriviallyCopyableType arr[3] = {
      TriviallyCopyableType{extractor.get_int32(), extractor.get_double(), extractor.get_uint32()},
      TriviallyCopyableType{extractor.get_int32(), extractor.get_double(), extractor.get_uint32()},
      TriviallyCopyableType{extractor.get_int32(), extractor.get_double(), extractor.get_uint32()}};
    LOG_INFO(g_logger, "TC Array: {}", arr);
    break;
  }
  case 4:
  {
    // Test vector of trivially copyable
    std::vector<TriviallyCopyableType> vec;
    size_t vec_size = extractor.get_byte() % 5;
    for (size_t i = 0; i < vec_size; ++i)
    {
      vec.emplace_back(extractor.get_int32(), extractor.get_double(), extractor.get_uint32());
    }
    LOG_INFO(g_logger, "TC Vector: {}", vec);
    break;
  }
  case 5:
  {
    // Test vector of complex type
    std::vector<ComplexType> vec;
    size_t vec_size = extractor.get_byte() % 3;
    for (size_t i = 0; i < vec_size; ++i)
    {
      vec.emplace_back(extractor.get_string(16), extractor.get_int32());
    }
    LOG_INFO(g_logger, "Complex Vector: {}", vec);
    break;
  }
  case 6:
  {
    // Test std::array of complex type
    std::array<ComplexType, 2> arr;
    arr[0] = ComplexType{extractor.get_string(16), extractor.get_int32()};
    arr[1] = ComplexType{extractor.get_string(16), extractor.get_int32()};
    LOG_INFO(g_logger, "Complex Array: {}", arr);
    break;
  }
  case 7:
  {
    // Test std::deque of complex type
    std::deque<ComplexType> deq;
    size_t deq_size = extractor.get_byte() % 3;
    for (size_t i = 0; i < deq_size; ++i)
    {
      deq.emplace_back(extractor.get_string(16), extractor.get_int32());
    }
    LOG_INFO(g_logger, "Complex Deque: {}", deq);
    break;
  }
  case 8:
  {
    // Test std::list of trivially copyable
    std::list<TriviallyCopyableType> lst;
    size_t list_size = extractor.get_byte() % 3;
    for (size_t i = 0; i < list_size; ++i)
    {
      lst.emplace_back(extractor.get_int32(), extractor.get_double(), extractor.get_uint32());
    }
    LOG_INFO(g_logger, "TC List: {}", lst);
    break;
  }
  case 9:
  {
    // Test std::map with complex value
    std::map<int, ComplexType> map_data;
    size_t map_size = extractor.get_byte() % 3;
    for (size_t i = 0; i < map_size; ++i)
    {
      map_data.emplace(extractor.get_int32(), ComplexType{extractor.get_string(16), extractor.get_int32()});
    }
    LOG_INFO(g_logger, "Complex Map: {}", map_data);
    break;
  }
  case 10:
  {
    // Test std::set of complex type
    std::set<ComplexType> set_data;
    size_t set_size = extractor.get_byte() % 3;
    for (size_t i = 0; i < set_size; ++i)
    {
      set_data.emplace(extractor.get_string(16), extractor.get_int32());
    }
    LOG_INFO(g_logger, "Complex Set: {}", set_data);
    break;
  }
  case 11:
  {
    // Test std::pair with complex types
    auto p = std::make_pair(
      ComplexType{extractor.get_string(16), extractor.get_int32()},
      TriviallyCopyableType{extractor.get_int32(), extractor.get_double(), extractor.get_uint32()});
    LOG_INFO(g_logger, "Pair: {}", p);
    break;
  }
  case 12:
  {
    // Test LOGV with user-defined types
    TriviallyCopyableType tc{extractor.get_int32(), extractor.get_double(), extractor.get_uint32()};
    ComplexType cx{extractor.get_string(16), extractor.get_int32()};
    LOGV_INFO(g_logger, "LOGV test", tc, cx);
    break;
  }
  case 13:
  {
    // Test mixed: basic + custom types
    int val = extractor.get_int32();
    TriviallyCopyableType tc{extractor.get_int32(), extractor.get_double(), extractor.get_uint32()};
    std::string str = extractor.get_string(16);
    LOG_INFO(g_logger, "Mixed: {} {} {}", val, tc, str);
    break;
  }
  case 14:
  {
    // Test nested containers
    std::vector<std::pair<int, TriviallyCopyableType>> vec_pair;
    size_t vec_size = extractor.get_byte() % 3;
    for (size_t i = 0; i < vec_size; ++i)
    {
      vec_pair.emplace_back(
        extractor.get_int32(),
        TriviallyCopyableType{extractor.get_int32(), extractor.get_double(), extractor.get_uint32()});
    }
    LOG_INFO(g_logger, "Vector of Pairs: {}", vec_pair);
    break;
  }
  case 15:
  {
    // Test large type (1KB)
    LargeType large{extractor.get_int32(), extractor.get_uint32()};
    LOG_INFO(g_logger, "Large: {}", large);
    break;
  }
  case 16:
  {
    // Test empty type
    EmptyType empty;
    LOG_INFO(g_logger, "Empty: {}", empty);
    break;
  }
  case 17:
  {
    // Test unaligned type
    UnalignedType unaligned{extractor.get_byte(), extractor.get_uint32(), extractor.get_byte(),
                            extractor.get_uint32()};
    LOG_INFO(g_logger, "Unaligned: {}", unaligned);
    break;
  }
  case 18:
  {
    // Test very large string in custom type (up to 64KB)
    size_t str_size = extractor.get_uint32() % 65536;
    std::string large_str;
    large_str.reserve(str_size);
    while (large_str.size() < str_size && extractor.has_data())
    {
      large_str.push_back(static_cast<char>(extractor.get_byte()));
    }
    VeryLargeStringType very_large{std::move(large_str), extractor.get_int32()};
    LOG_INFO(g_logger, "VeryLargeStr: {}", very_large);
    break;
  }
  case 19:
  {
    // Test custom type with empty containers
    ComplexType empty_complex{"", 0};
    empty_complex.tags.clear();
    LOG_INFO(g_logger, "EmptyComplex: {}", empty_complex);
    break;
  }
  case 20:
  {
    // Test unordered_set with custom types
    std::unordered_set<int> uset;
    size_t uset_size = extractor.get_byte() % 5;
    for (size_t i = 0; i < uset_size; ++i)
    {
      uset.insert(extractor.get_int32());
    }
    LOG_INFO(g_logger, "UnorderedSet: {}", uset);
    break;
  }
  case 21:
  {
    // Test deeply nested: map<int, vector<ComplexType>>
    std::map<int, std::vector<ComplexType>> deep_nested;
    size_t map_size = extractor.get_byte() % 2;
    for (size_t i = 0; i < map_size; ++i)
    {
      int key = extractor.get_int32();
      std::vector<ComplexType> vec;
      size_t vec_size = extractor.get_byte() % 2;
      for (size_t j = 0; j < vec_size; ++j)
      {
        vec.emplace_back(extractor.get_string(8), extractor.get_int32());
      }
      deep_nested[key] = std::move(vec);
    }
    LOG_INFO(g_logger, "DeepNested: {}", deep_nested);
    break;
  }
  case 22:
  {
    // Test multiple (5) custom types in one log
    TriviallyCopyableType tc1{extractor.get_int32(), extractor.get_double(), extractor.get_uint32()};
    TriviallyCopyableType tc2{extractor.get_int32(), extractor.get_double(), extractor.get_uint32()};
    ComplexType cx{extractor.get_string(8), extractor.get_int32()};
    UnalignedType ua{extractor.get_byte(), extractor.get_uint32(), extractor.get_byte(), extractor.get_uint32()};
    EmptyType empty;
    LOG_INFO(g_logger, "Multi5: {} {} {} {} {}", tc1, tc2, cx, ua, empty);
    break;
  }
  case 23:
  {
    // Test vector of large types
    std::vector<LargeType> vec_large;
    size_t vec_size = extractor.get_byte() % 3; // Limited to 3 due to size
    for (size_t i = 0; i < vec_size; ++i)
    {
      vec_large.emplace_back(extractor.get_int32(), extractor.get_uint32());
    }
    LOG_INFO(g_logger, "VectorLarge: {}", vec_large);
    break;
  }
  }

  return 0;
}
