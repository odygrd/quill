#include "doctest/doctest.h"

#include "quill/core/BufferUtilities.h"

#include "quill/bundled/fmt/format.h"

#include <cstddef>
#include <deque>
#include <iterator>
#include <string>

TEST_SUITE_BEGIN("BufferUtilities");

using namespace quill::detail;

namespace
{
/** Text of every length up to and past the 16 bytes that are copied without memcpy */
std::string make_text(size_t size)
{
  std::string text;

  for (size_t i = 0; i < size; ++i)
  {
    text.push_back(static_cast<char>('a' + (i % 26)));
  }

  return text;
}
} // namespace

/***/
TEST_CASE("append_to_buffer_appends_text_of_every_size")
{
  fmtquill::memory_buffer buffer;
  std::string expected;

  for (size_t size = 0; size <= 40; ++size)
  {
    std::string const text = make_text(size);
    append_to_buffer(buffer, text);
    expected += text;

    REQUIRE_EQ(std::string{buffer.data(), buffer.size()}, expected);
  }
}

/***/
TEST_CASE("append_to_buffer_grows_the_buffer")
{
  // A small inline capacity, so that most appends need the buffer to grow
  fmtquill::basic_memory_buffer<char, 4> buffer;
  std::string expected;

  for (size_t size = 0; size <= 40; ++size)
  {
    std::string const text = make_text(size);
    append_to_buffer(buffer, text);
    expected += text;
  }

  REQUIRE_EQ(std::string{buffer.data(), buffer.size()}, expected);
}

/***/
TEST_CASE("append_to_buffer_flushes_a_buffer_with_a_fixed_capacity")
{
  // For an output iterator into a container that is not contiguous, fmt formats into a buffer
  // with a fixed capacity that is flushed to the iterator when full, instead of growing
  std::deque<char> output;
  std::string expected;

  {
    fmtquill::detail::iterator_buffer<std::back_insert_iterator<std::deque<char>>, char> buffer{
      std::back_inserter(output)};

    for (size_t round = 0; round < 20; ++round)
    {
      for (size_t size = 0; size <= 40; ++size)
      {
        std::string const text = make_text(size);
        append_to_buffer(buffer, text);
        expected += text;
      }
    }

    // The destructor flushes the rest
  }

  REQUIRE_EQ(std::string(output.begin(), output.end()), expected);
}

TEST_SUITE_END();
