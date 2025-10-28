#define FUZZER_LOG_FILENAME "stl_containers_fuzz.log"
#include "FuzzerHelper.h"

#include "quill/LogMacros.h"

#include "quill/std/Array.h"
#include "quill/std/Chrono.h"
#include "quill/std/Deque.h"
#include "quill/std/FilesystemPath.h"
#include "quill/std/ForwardList.h"
#include "quill/std/List.h"
#include "quill/std/Map.h"
#include "quill/std/Optional.h"
#include "quill/std/Pair.h"
#include "quill/std/Set.h"
#include "quill/std/Tuple.h"
#include "quill/std/UnorderedMap.h"
#include "quill/std/UnorderedSet.h"
#include "quill/std/Vector.h"
#include "quill/std/WideString.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <filesystem>
#include <forward_list>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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

  std::wstring get_wstring(size_t max_len = 64)
  {
    if (_offset >= _size)
      return L"";

    size_t len = get_byte() % (max_len + 1);
    std::wstring result;
    for (size_t i = 0; i < len && _offset < _size; ++i)
    {
      result.push_back(static_cast<wchar_t>(get_byte()));
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

  switch (selector % 20)
  {
  case 0:
  {
    // Test std::vector
    std::vector<int> vec;
    size_t vec_size = extractor.get_byte() % 10;
    for (size_t i = 0; i < vec_size; ++i)
    {
      vec.push_back(extractor.get_int32());
    }
    LOG_INFO(g_logger, "Vector: {}", vec);
    break;
  }
  case 1:
  {
    // Test std::array
    std::array<int, 5> arr;
    for (size_t i = 0; i < arr.size(); ++i)
    {
      arr[i] = extractor.get_int32();
    }
    LOG_INFO(g_logger, "Array: {}", arr);
    break;
  }
  case 2:
  {
    // Test std::deque
    std::deque<std::string> deq;
    size_t deq_size = extractor.get_byte() % 5;
    for (size_t i = 0; i < deq_size; ++i)
    {
      deq.push_back(extractor.get_string(32));
    }
    LOG_INFO(g_logger, "Deque: {}", deq);
    break;
  }
  case 3:
  {
    // Test std::list
    std::list<double> lst;
    size_t list_size = extractor.get_byte() % 5;
    for (size_t i = 0; i < list_size; ++i)
    {
      lst.push_back(extractor.get_double());
    }
    LOG_INFO(g_logger, "List: {}", lst);
    break;
  }
  case 4:
  {
    // Test std::forward_list
    std::forward_list<int> flist;
    size_t flist_size = extractor.get_byte() % 5;
    for (size_t i = 0; i < flist_size; ++i)
    {
      flist.push_front(extractor.get_int32());
    }
    LOG_INFO(g_logger, "ForwardList: {}", flist);
    break;
  }
  case 5:
  {
    // Test std::map
    std::map<int, std::string> map_data;
    size_t map_size = extractor.get_byte() % 5;
    for (size_t i = 0; i < map_size; ++i)
    {
      int key = extractor.get_int32();
      std::string value = extractor.get_string(32);
      map_data[key] = value;
    }
    LOG_INFO(g_logger, "Map: {}", map_data);
    break;
  }
  case 6:
  {
    // Test std::unordered_map
    std::unordered_map<int, double> umap;
    size_t umap_size = extractor.get_byte() % 5;
    for (size_t i = 0; i < umap_size; ++i)
    {
      umap[extractor.get_int32()] = extractor.get_double();
    }
    LOG_INFO(g_logger, "UnorderedMap: {}", umap);
    break;
  }
  case 7:
  {
    // Test std::set
    std::set<int> set_data;
    size_t set_size = extractor.get_byte() % 5;
    for (size_t i = 0; i < set_size; ++i)
    {
      set_data.insert(extractor.get_int32());
    }
    LOG_INFO(g_logger, "Set: {}", set_data);
    break;
  }
  case 8:
  {
    // Test std::unordered_set
    std::unordered_set<int> uset;
    size_t uset_size = extractor.get_byte() % 5;
    for (size_t i = 0; i < uset_size; ++i)
    {
      uset.insert(extractor.get_int32());
    }
    LOG_INFO(g_logger, "UnorderedSet: {}", uset);
    break;
  }
  case 9:
  {
    // Test std::pair
    int first = extractor.get_int32();
    std::string second = extractor.get_string(32);
    std::pair<int, std::string> p{first, second};
    LOG_INFO(g_logger, "Pair: {}", p);
    break;
  }
  case 10:
  {
    // Test std::tuple
    int a = extractor.get_int32();
    double b = extractor.get_double();
    std::string c = extractor.get_string(32);
    auto tpl = std::make_tuple(a, b, c);
    LOG_INFO(g_logger, "Tuple: {}", tpl);
    break;
  }
  case 11:
  {
    // Test std::optional with value
    std::optional<int> opt = extractor.get_int32();
    LOG_INFO(g_logger, "Optional with value: {}", opt);
    break;
  }
  case 12:
  {
    // Test std::optional empty
    std::optional<int> opt;
    LOG_INFO(g_logger, "Optional empty: {}", opt);
    break;
  }
  case 13:
  {
    // Test std::chrono::duration
    auto duration = std::chrono::milliseconds{extractor.get_uint64()};
    LOG_INFO(g_logger, "Duration: {}", duration);
    break;
  }
  case 14:
  {
    // Test std::chrono::time_point
    auto now = std::chrono::system_clock::now();
    LOG_INFO(g_logger, "TimePoint: {}", now);
    break;
  }
  case 15:
  {
    // Test std::filesystem::path
    std::string path_str = extractor.get_string(64);
    std::filesystem::path fspath{path_str};
    LOG_INFO(g_logger, "FilesystemPath: {}", fspath);
    break;
  }
  case 16:
  {
    // Test std::chrono::seconds
    auto seconds = std::chrono::seconds{extractor.get_uint64() % 86400};
    LOG_INFO(g_logger, "Seconds: {}", seconds);
    break;
  }
  case 17:
  {
    // Test nested containers - vector of vectors
    std::vector<std::vector<int>> nested;
    size_t outer_size = extractor.get_byte() % 3;
    for (size_t i = 0; i < outer_size; ++i)
    {
      std::vector<int> inner;
      size_t inner_size = extractor.get_byte() % 3;
      for (size_t j = 0; j < inner_size; ++j)
      {
        inner.push_back(extractor.get_int32());
      }
      nested.push_back(inner);
    }
    LOG_INFO(g_logger, "Nested vector: {}", nested);
    break;
  }
  case 18:
  {
    // Test map of pairs
    std::map<int, std::pair<std::string, double>> complex_map;
    size_t map_size = extractor.get_byte() % 3;
    for (size_t i = 0; i < map_size; ++i)
    {
      int key = extractor.get_int32();
      std::string str = extractor.get_string(16);
      double val = extractor.get_double();
      complex_map[key] = std::make_pair(str, val);
    }
    LOG_INFO(g_logger, "Complex map: {}", complex_map);
    break;
  }
  case 19:
  {
    // Test vector of strings
    std::vector<std::string> vec_str;
    size_t vec_size = extractor.get_byte() % 5;
    for (size_t i = 0; i < vec_size; ++i)
    {
      vec_str.push_back(extractor.get_string(16));
    }
    LOG_INFO(g_logger, "Vector of strings: {}", vec_str);
    break;
  }
  }

  return 0;
}
