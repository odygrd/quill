#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

// Shared helper to extract typed data from fuzzer input.
// Used by all fuzz targets for consistent data consumption.
class FuzzDataExtractor
{
public:
  explicit FuzzDataExtractor(uint8_t const* data, size_t size)
    : _data(data), _size(size), _offset(0)
  {
  }

  bool has_data() const { return _offset < _size; }

  size_t remaining() const { return _size - _offset; }

  uint8_t get_byte()
  {
    if (_offset < _size)
    {
      return _data[_offset++];
    }
    return 0;
  }

  int8_t get_int8() { return static_cast<int8_t>(get_byte()); }

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

  int16_t get_int16() { return static_cast<int16_t>(get_uint16()); }

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

  int64_t get_int64() { return static_cast<int64_t>(get_uint64()); }

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

  std::string get_string(size_t max_len = 256)
  {
    if (_offset >= _size)
    {
      return "";
    }

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
    {
      return L"";
    }

    size_t len = get_byte() % (max_len + 1);
    std::wstring result;
    for (size_t i = 0; i < len && _offset < _size; ++i)
    {
      result.push_back(static_cast<wchar_t>(get_byte()));
    }
    return result;
  }

  std::string_view get_string_view()
  {
    if (_offset >= _size)
    {
      return "";
    }

    size_t len = get_byte();
    if (_offset + len > _size)
    {
      len = _size - _offset;
    }

    std::string_view result(reinterpret_cast<char const*>(_data + _offset), len);
    _offset += len;
    return result;
  }

  // Get null-terminated C string from fuzzer data.
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

  // Extract a string of specified length from fuzzer data
  std::string get_bytes(size_t length)
  {
    if (_offset >= _size || length == 0)
    {
      return "";
    }

    // Clamp to remaining data
    size_t available = _size - _offset;
    size_t actual_len = (length < available) ? length : available;

    std::string result(reinterpret_cast<char const*>(_data + _offset), actual_len);
    _offset += actual_len;
    return result;
  }

  // Get raw bytes as std::byte pointer (for BinaryData)
  std::byte const* get_binary_data(size_t length)
  {
    if (_offset >= _size || length == 0)
    {
      return nullptr;
    }

    // Clamp to remaining data
    size_t available = _size - _offset;
    size_t actual_len = (length < available) ? length : available;

    std::byte const* result = reinterpret_cast<std::byte const*>(_data + _offset);
    _offset += actual_len;
    return result;
  }

private:
  uint8_t const* _data;
  size_t _size;
  size_t _offset;
  std::array<char, 512> _c_string_buffer{}; // Buffer for null-terminated C strings
};
