#include "doctest/doctest.h"

#include "quill/core/Codec.h"

#include <array>
#include <cstddef>
#include <cstring>

TEST_SUITE_BEGIN("Codec");

using namespace quill::detail;

TEST_CASE("copy_string_bytes_copies_every_length_without_writing_past_it")
{
  std::array<char, 160> source{};

  for (size_t i = 0; i < source.size(); ++i)
  {
    source[i] = static_cast<char>('A' + (i % 26u));
  }

  // Every branch: 0, 1-3, 4-7, 8-16, 17-32, 33-64 and the memcpy fallback above 64 bytes
  for (size_t len = 0; len <= 130u; ++len)
  {
    std::array<std::byte, 192> destination{};
    std::memset(destination.data(), 0x5A, destination.size());

    // Copy to an odd offset so the fixed-size copies are not aligned
    std::byte* const dst = destination.data() + 3u;
    copy_string_bytes(dst, source.data() + 1u, len);

    REQUIRE_EQ(std::memcmp(dst, source.data() + 1u, len), 0);

    // The bytes before and after the copy are untouched
    for (size_t i = 0; i < 3u; ++i)
    {
      REQUIRE_EQ(destination[i], std::byte{0x5A});
    }

    for (size_t i = 3u + len; i < destination.size(); ++i)
    {
      REQUIRE_EQ(destination[i], std::byte{0x5A});
    }
  }
}

TEST_SUITE_END();
