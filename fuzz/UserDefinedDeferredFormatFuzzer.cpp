#include "quill/Backend.h"
#include "quill/DeferredFormatCodec.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/core/Codec.h"
#include "quill/sinks/FileSink.h"

#include "quill/std/Array.h"
#include "quill/std/Deque.h"
#include "quill/std/List.h"
#include "quill/std/Map.h"
#include "quill/std/Pair.h"
#include "quill/std/Set.h"
#include "quill/std/Vector.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
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

static quill::Logger* g_logger = nullptr;
static bool g_initialized = false;

extern "C" int LLVMFuzzerInitialize(int* /*argc*/, char*** /*argv*/)
{
  if (!g_initialized)
  {
    quill::BackendOptions backend_options;
    backend_options.error_notifier = [](std::string const&) {};
    quill::Backend::start(backend_options);

    auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
      "user_defined_deferred_fuzz.log",
      []()
      {
        quill::FileSinkConfig cfg;
        cfg.set_open_mode('w');
        cfg.set_filename_append_option(quill::FilenameAppendOption::None);
        return cfg;
      }(),
      quill::FileEventNotifier{});

    g_logger = quill::Frontend::create_or_get_logger("deferred_format_fuzzer", std::move(file_sink));
    g_logger->set_log_level(quill::LogLevel::TraceL3);
    g_logger->set_immediate_flush(250);
    g_initialized = true;
  }
  return 0;
}

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
  uint8_t log_level_selector = extractor.get_byte();

  switch (log_level_selector % 5)
  {
  case 0:
    g_logger->set_log_level(quill::LogLevel::Debug);
    break;
  case 1:
    g_logger->set_log_level(quill::LogLevel::Info);
    break;
  case 2:
    g_logger->set_log_level(quill::LogLevel::Warning);
    break;
  case 3:
    g_logger->set_log_level(quill::LogLevel::Error);
    break;
  default:
    g_logger->set_log_level(quill::LogLevel::Critical);
    break;
  }

  switch (selector % 15)
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
  }

  return 0;
}
