#define FUZZER_LOG_FILENAME "user_defined_direct_fuzz.log"
#include "FuzzerHelper.h"

#include "quill/DirectFormatCodec.h"
#include "quill/HelperMacros.h"
#include "quill/LogMacros.h"
#include "quill/core/Codec.h"

#include "quill/std/Array.h"
#include "quill/std/Deque.h"
#include "quill/std/List.h"
#include "quill/std/Map.h"
#include "quill/std/Set.h"
#include "quill/std/UnorderedMap.h"
#include "quill/std/UnorderedSet.h"
#include "quill/std/Vector.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ostream>
#include <string>
#include <unordered_set>
#include <vector>

// Trivially copyable custom type for DirectFormat
struct DirectFormatType
{
  DirectFormatType(int id, double value) : id(id), value(value) {}

  int id;
  double value;
};

template <>
struct fmtquill::formatter<DirectFormatType>
{
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(::DirectFormatType const& obj, FormatContext& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "DF{{id: {}, value: {}}}", obj.id, obj.value);
  }
};

template <>
struct quill::Codec<DirectFormatType> : quill::DirectFormatCodec<DirectFormatType>
{
};

// Complex type with std::string for DirectFormat
class DirectComplexType
{
public:
  DirectComplexType() = default;

  DirectComplexType(std::string name, int count) : name(std::move(name)), count(count)
  {
    labels.push_back("label1");
    labels.push_back("label2");
  }

  bool operator<(DirectComplexType const& other) const { return this->count < other.count; }

  bool operator==(DirectComplexType const& other) const
  {
    return this->count == other.count && this->name == other.name;
  }

  std::string name;
  int count{};
  std::vector<std::string> labels;
};

namespace std
{
template <>
struct hash<DirectComplexType>
{
  std::size_t operator()(DirectComplexType const& obj) const { return std::hash<int>()(obj.count); }
};
} // namespace std

template <>
struct fmtquill::formatter<DirectComplexType>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::DirectComplexType const& obj, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "DCT{{name: {}, count: {}, labels: {}}}", obj.name,
                               obj.count, obj.labels);
  }
};

template <>
struct quill::Codec<DirectComplexType> : quill::DirectFormatCodec<DirectComplexType>
{
};

// Enum for DirectFormat
enum class Priority
{
  Trace,
  Debug,
  Info,
  Warn,
  Error
};

std::ostream& operator<<(std::ostream& os, Priority const& level)
{
  switch (level)
  {
  case Priority::Trace:
    os << "TRACE";
    break;
  case Priority::Debug:
    os << "DEBUG";
    break;
  case Priority::Info:
    os << "INFO";
    break;
  case Priority::Warn:
    os << "WARN";
    break;
  case Priority::Error:
    os << "ERROR";
    break;
  }
  return os;
}

QUILL_LOGGABLE_DIRECT_FORMAT(Priority)

// Scoped enum
enum class Status : uint8_t
{
  Success = 0,
  Failure = 1,
  Pending = 2
};

std::ostream& operator<<(std::ostream& os, Status const& status)
{
  switch (status)
  {
  case Status::Success:
    os << "Success";
    break;
  case Status::Failure:
    os << "Failure";
    break;
  case Status::Pending:
    os << "Pending";
    break;
  }
  return os;
}

QUILL_LOGGABLE_DIRECT_FORMAT(Status)

// Large DirectFormat type (1KB)
struct LargeDirectType
{
  LargeDirectType() = default;
  LargeDirectType(int id, uint32_t fill_val) : id(id)
  {
    std::fill(std::begin(large_buffer), std::end(large_buffer), static_cast<uint8_t>(fill_val));
  }

  int id{};
  uint8_t large_buffer[1020]{};
};

template <>
struct fmtquill::formatter<LargeDirectType>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::LargeDirectType const& obj, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "LargeDirect{{id:{}, size:{}}}", obj.id, sizeof(obj.large_buffer));
  }
};

template <>
struct quill::Codec<LargeDirectType> : quill::DirectFormatCodec<LargeDirectType>
{
};

// Empty DirectFormat type
struct EmptyDirectType
{
  EmptyDirectType() = default;
};

template <>
struct fmtquill::formatter<EmptyDirectType>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::EmptyDirectType const&, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "EmptyDirect{{}}");
  }
};

template <>
struct quill::Codec<EmptyDirectType> : quill::DirectFormatCodec<EmptyDirectType>
{
};

// Unaligned DirectFormat type
struct UnalignedDirectType
{
  UnalignedDirectType() = default;
  UnalignedDirectType(uint8_t a, uint64_t b, uint8_t c) : a(a), b(b), c(c) {}

  uint8_t a{};
  uint64_t b{};
  uint8_t c{};
};

template <>
struct fmtquill::formatter<UnalignedDirectType>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::UnalignedDirectType const& obj, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "UnalignedDirect{{a:{}, b:{}, c:{}}}", obj.a, obj.b, obj.c);
  }
};

template <>
struct quill::Codec<UnalignedDirectType> : quill::DirectFormatCodec<UnalignedDirectType>
{
};

// DirectFormat type with very large string
struct VeryLargeStringDirectType
{
  VeryLargeStringDirectType() = default;
  VeryLargeStringDirectType(std::string huge_str, int id) : huge_str(std::move(huge_str)), id(id) {}

  std::string huge_str;
  int id{};
};

template <>
struct fmtquill::formatter<VeryLargeStringDirectType>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::VeryLargeStringDirectType const& obj, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "VeryLargeDirect{{id:{}, len:{}}}", obj.id, obj.huge_str.size());
  }
};

template <>
struct quill::Codec<VeryLargeStringDirectType> : quill::DirectFormatCodec<VeryLargeStringDirectType>
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
    {
      return _data[_offset++];
    }
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

  switch (selector % 27)
  {
  case 0:
  {
    // Test DirectFormatType
    DirectFormatType obj{extractor.get_int32(), extractor.get_double()};
    LOG_INFO(g_logger, "DF: {}", obj);
    break;
  }
  case 1:
  {
    // Test DirectComplexType
    DirectComplexType obj{extractor.get_string(32), extractor.get_int32()};
    LOG_INFO(g_logger, "DCT: {}", obj);
    break;
  }
  case 2:
  {
    // Test Priority enum
    Priority level = static_cast<Priority>(extractor.get_byte() % 5);
    LOG_INFO(g_logger, "Priority: {}", level);
    break;
  }
  case 3:
  {
    // Test Status enum
    Status status = static_cast<Status>(extractor.get_byte() % 3);
    LOG_INFO(g_logger, "Status: {}", status);
    break;
  }
  case 4:
  {
    // Test array of DirectFormatType
    DirectFormatType arr[3] = {DirectFormatType{extractor.get_int32(), extractor.get_double()},
                               DirectFormatType{extractor.get_int32(), extractor.get_double()},
                               DirectFormatType{extractor.get_int32(), extractor.get_double()}};
    LOG_INFO(g_logger, "DF Array: {}", arr);
    break;
  }
  case 5:
  {
    // Test vector of DirectFormatType
    std::vector<DirectFormatType> vec;
    size_t vec_size = extractor.get_byte() % 5;
    for (size_t i = 0; i < vec_size; ++i)
    {
      vec.emplace_back(extractor.get_int32(), extractor.get_double());
    }
    LOG_INFO(g_logger, "DF Vector: {}", vec);
    break;
  }
  case 6:
  {
    // Test vector of DirectComplexType
    std::vector<DirectComplexType> vec;
    size_t vec_size = extractor.get_byte() % 3;
    for (size_t i = 0; i < vec_size; ++i)
    {
      vec.emplace_back(extractor.get_string(16), extractor.get_int32());
    }
    LOG_INFO(g_logger, "DCT Vector: {}", vec);
    break;
  }
  case 7:
  {
    // Test std::array of DirectComplexType
    std::array<DirectComplexType, 2> arr;
    arr[0] = DirectComplexType{extractor.get_string(16), extractor.get_int32()};
    arr[1] = DirectComplexType{extractor.get_string(16), extractor.get_int32()};
    LOG_INFO(g_logger, "DCT Array: {}", arr);
    break;
  }
  case 8:
  {
    // Test std::deque
    std::deque<DirectFormatType> deq;
    size_t deq_size = extractor.get_byte() % 3;
    for (size_t i = 0; i < deq_size; ++i)
    {
      deq.emplace_back(extractor.get_int32(), extractor.get_double());
    }
    LOG_INFO(g_logger, "DF Deque: {}", deq);
    break;
  }
  case 9:
  {
    // Test std::list
    std::list<DirectComplexType> lst;
    size_t list_size = extractor.get_byte() % 3;
    for (size_t i = 0; i < list_size; ++i)
    {
      lst.emplace_back(extractor.get_string(16), extractor.get_int32());
    }
    LOG_INFO(g_logger, "DCT List: {}", lst);
    break;
  }
  case 10:
  {
    // Test std::map
    std::map<int, DirectComplexType> map_data;
    size_t map_size = extractor.get_byte() % 3;
    for (size_t i = 0; i < map_size; ++i)
    {
      map_data.emplace(extractor.get_int32(),
                       DirectComplexType{extractor.get_string(16), extractor.get_int32()});
    }
    LOG_INFO(g_logger, "DCT Map: {}", map_data);
    break;
  }
  case 11:
  {
    // Test std::set
    std::set<DirectComplexType> set_data;
    size_t set_size = extractor.get_byte() % 3;
    for (size_t i = 0; i < set_size; ++i)
    {
      set_data.emplace(extractor.get_string(16), extractor.get_int32());
    }
    LOG_INFO(g_logger, "DCT Set: {}", set_data);
    break;
  }
  case 12:
  {
    // Test std::unordered_map
    std::unordered_map<int, DirectComplexType> umap;
    size_t umap_size = extractor.get_byte() % 3;
    for (size_t i = 0; i < umap_size; ++i)
    {
      umap.emplace(extractor.get_int32(), DirectComplexType{extractor.get_string(16), extractor.get_int32()});
    }
    LOG_INFO(g_logger, "DCT UnorderedMap: {}", umap);
    break;
  }
  case 13:
  {
    // Test vector of enums
    std::vector<Priority> vec_enum;
    size_t vec_size = extractor.get_byte() % 5;
    for (size_t i = 0; i < vec_size; ++i)
    {
      vec_enum.push_back(static_cast<Priority>(extractor.get_byte() % 5));
    }
    LOG_INFO(g_logger, "Priority Vector: {}", vec_enum);
    break;
  }
  case 14:
  {
    // Test mixed: enum + custom type
    Status status = static_cast<Status>(extractor.get_byte() % 3);
    DirectFormatType df{extractor.get_int32(), extractor.get_double()};
    LOG_INFO(g_logger, "Mixed: {} {}", status, df);
    break;
  }
  case 15:
  {
    // Test LOGV with DirectFormat types
    DirectFormatType df{extractor.get_int32(), extractor.get_double()};
    Priority level = static_cast<Priority>(extractor.get_byte() % 5);
    LOGV_INFO(g_logger, "LOGV test", df, level);
    break;
  }
  case 16:
  {
    // Test all enum values
    LOG_INFO(g_logger, "All Priorities: {} {} {} {} {}", Priority::Trace, Priority::Debug,
             Priority::Info, Priority::Warn, Priority::Error);
    break;
  }
  case 17:
  {
    // Test nested: vector of pairs
    std::vector<std::pair<Status, DirectFormatType>> vec_pair;
    size_t vec_size = extractor.get_byte() % 3;
    for (size_t i = 0; i < vec_size; ++i)
    {
      vec_pair.emplace_back(static_cast<Status>(extractor.get_byte() % 3),
                            DirectFormatType{extractor.get_int32(), extractor.get_double()});
    }
    LOG_INFO(g_logger, "Vector of Pairs: {}", vec_pair);
    break;
  }
  case 18:
  {
    // Test large DirectFormat type (1KB)
    LargeDirectType large{extractor.get_int32(), extractor.get_uint32()};
    LOG_INFO(g_logger, "LargeDirect: {}", large);
    break;
  }
  case 19:
  {
    // Test empty DirectFormat type
    EmptyDirectType empty;
    LOG_INFO(g_logger, "EmptyDirect: {}", empty);
    break;
  }
  case 20:
  {
    // Test unaligned DirectFormat type
    UnalignedDirectType unaligned{extractor.get_byte(), extractor.get_uint32(), extractor.get_byte()};
    LOG_INFO(g_logger, "UnalignedDirect: {}", unaligned);
    break;
  }
  case 21:
  {
    // Test very large string in DirectFormat type (up to 64KB)
    size_t str_size = extractor.get_uint32() % 65536;
    std::string huge_str;
    huge_str.reserve(str_size);
    while (huge_str.size() < str_size && extractor.has_data())
    {
      huge_str.push_back(static_cast<char>(extractor.get_byte()));
    }
    VeryLargeStringDirectType very_large{std::move(huge_str), extractor.get_int32()};
    LOG_INFO(g_logger, "VeryLargeDirect: {}", very_large);
    break;
  }
  case 22:
  {
    // Test unordered_set with enums
    std::unordered_set<int> uset;
    size_t uset_size = extractor.get_byte() % 5;
    for (size_t i = 0; i < uset_size; ++i)
    {
      uset.insert(extractor.get_int32());
    }
    LOG_INFO(g_logger, "UnorderedSetInt: {}", uset);
    break;
  }
  case 23:
  {
    // Test deeply nested: map<Priority, vector<DirectFormatType>>
    std::map<int, std::vector<DirectFormatType>> deep_nested;
    size_t map_size = extractor.get_byte() % 2;
    for (size_t i = 0; i < map_size; ++i)
    {
      int key = extractor.get_int32();
      std::vector<DirectFormatType> vec;
      size_t vec_size = extractor.get_byte() % 2;
      for (size_t j = 0; j < vec_size; ++j)
      {
        vec.emplace_back(extractor.get_int32(), extractor.get_double());
      }
      deep_nested[key] = std::move(vec);
    }
    LOG_INFO(g_logger, "DeepNestedDirect: {}", deep_nested);
    break;
  }
  case 24:
  {
    // Test multiple (6) DirectFormat types in one log
    DirectFormatType df1{extractor.get_int32(), extractor.get_double()};
    DirectFormatType df2{extractor.get_int32(), extractor.get_double()};
    Priority p = static_cast<Priority>(extractor.get_byte() % 5);
    Status s = static_cast<Status>(extractor.get_byte() % 3);
    UnalignedDirectType ua{extractor.get_byte(), extractor.get_uint32(), extractor.get_byte()};
    EmptyDirectType empty;
    LOG_INFO(g_logger, "Multi6: {} {} {} {} {} {}", df1, df2, p, s, ua, empty);
    break;
  }
  case 25:
  {
    // Test DirectComplexType with empty containers
    DirectComplexType empty_complex;
    empty_complex.name = "";
    empty_complex.labels.clear();
    LOG_INFO(g_logger, "EmptyDirectComplex: {}", empty_complex);
    break;
  }
  case 26:
  {
    // Test vector of large DirectFormat types
    std::vector<LargeDirectType> vec_large;
    size_t vec_size = extractor.get_byte() % 2; // Limited to 2 due to size
    for (size_t i = 0; i < vec_size; ++i)
    {
      vec_large.emplace_back(extractor.get_int32(), extractor.get_uint32());
    }
    LOG_INFO(g_logger, "VectorLargeDirect: {}", vec_large);
    break;
  }
  }

  return 0;
}
