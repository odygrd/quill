#include "DocTestExtensions.h"
#include "doctest/doctest.h"

#include "quill/detail/backend/TransitEventBuffer.h"

TEST_SUITE_BEGIN("TransitEventBuffer");

using namespace quill;
using namespace quill::detail;

/***/
TEST_CASE("transit_event_bounded_buffer")
{
  BoundedTransitEventBuffer bte{4};
  REQUIRE_EQ(bte.capacity(), 4);

  for (size_t i = 0; i < 12; ++i)
  {
    REQUIRE_FALSE(bte.front());
    REQUIRE_EQ(bte.size(), 0);

    {
      TransitEvent* te1 = bte.back();
      REQUIRE(te1);
      te1->structured_kvs.clear();
      te1->structured_kvs.emplace_back(std::string{"test1"} + std::to_string(i), "");
      bte.push_back();
    }

    REQUIRE_EQ(bte.size(), 1);

    {
      TransitEvent* te2 = bte.back();
      REQUIRE(te2);
      te2->structured_kvs.clear();
      te2->structured_kvs.emplace_back(std::string{"test2"} + std::to_string(i), "");
      bte.push_back();
    }

    REQUIRE_EQ(bte.size(), 2);

    {
      TransitEvent* te3 = bte.back();
      REQUIRE(te3);
      te3->structured_kvs.clear();
      te3->structured_kvs.emplace_back(std::string{"test3"} + std::to_string(i), "");
      bte.push_back();
    }

    REQUIRE_EQ(bte.size(), 3);

    {
      TransitEvent* te4 = bte.back();
      REQUIRE(te4);
      te4->structured_kvs.clear();
      te4->structured_kvs.emplace_back(std::string{"test4"} + std::to_string(i), "");
      bte.push_back();
    }

    REQUIRE_EQ(bte.size(), 4);

    // read
    {
      TransitEvent* te1 = bte.front();
      REQUIRE(te1);
      std::string const expected = std::string{"test1"} + std::to_string(i);
      REQUIRE_STREQ(te1->structured_kvs[0].first.data(), expected.data());
      bte.pop_front();
    }

    REQUIRE_EQ(bte.size(), 3);

    {
      TransitEvent* te2 = bte.front();
      REQUIRE(te2);
      std::string const expected = std::string{"test2"} + std::to_string(i);
      REQUIRE_STREQ(te2->structured_kvs[0].first.data(), expected.data());
      bte.pop_front();
    }

    REQUIRE_EQ(bte.size(), 2);

    {
      TransitEvent* te3 = bte.front();
      REQUIRE(te3);
      std::string const expected = std::string{"test3"} + std::to_string(i);
      REQUIRE_STREQ(te3->structured_kvs[0].first.data(), expected.data());
      bte.pop_front();
    }

    REQUIRE_EQ(bte.size(), 1);

    {
      TransitEvent* te4 = bte.front();
      REQUIRE(te4);
      std::string const expected = std::string{"test4"} + std::to_string(i);
      REQUIRE_STREQ(te4->structured_kvs[0].first.data(), expected.data());
      bte.pop_front();
    }

    REQUIRE_EQ(bte.size(), 0);
    REQUIRE_FALSE(bte.front());
  }
}

/***/
TEST_CASE("transit_event_bounded_buffer_integer_overflow")
{
  BoundedTransitEventBufferImpl<uint8_t> bte{128};
  size_t const iterations = static_cast<size_t>(std::numeric_limits<uint8_t>::max()) * 8ull;

  for (size_t i = 0; i < iterations; ++i)
  {
    REQUIRE_EQ(bte.size(), 0);

    {
      TransitEvent* te = bte.back();
      REQUIRE(te);
      te->structured_kvs.clear();
      te->structured_kvs.emplace_back(std::string{"test"} + std::to_string(i), "");
      bte.push_back();
    }

    REQUIRE_EQ(bte.size(), 1);

    // read
    {
      TransitEvent* te = bte.front();
      REQUIRE(te);
      std::string const expected = std::string{"test"} + std::to_string(i);
      REQUIRE_STREQ(te->structured_kvs[0].first.data(), expected.data());
      bte.pop_front();
    }
  }
}

/***/
TEST_CASE("transit_event_unbounded_buffer")
{
  UnboundedTransitEventBuffer bte{4};

  REQUIRE_FALSE(bte.front());
  REQUIRE(bte.empty());
  REQUIRE_EQ(bte.size(), 0);

  for (size_t i = 0; i < 128; ++i)
  {
    {
      TransitEvent* te1 = bte.back();
      REQUIRE(te1);
      te1->structured_kvs.clear();
      te1->structured_kvs.emplace_back(std::string{"test1"} + std::to_string(i), "");
      bte.push_back();
    }

    REQUIRE_EQ(bte.size(), i * 4 + 1);

    {
      TransitEvent* te2 = bte.back();
      REQUIRE(te2);
      te2->structured_kvs.clear();
      te2->structured_kvs.emplace_back(std::string{"test2"} + std::to_string(i), "");
      bte.push_back();
    }

    REQUIRE_EQ(bte.size(), i * 4 + 2);

    {
      TransitEvent* te3 = bte.back();
      REQUIRE(te3);
      te3->structured_kvs.clear();
      te3->structured_kvs.emplace_back(std::string{"test3"} + std::to_string(i), "");
      bte.push_back();
    }

    REQUIRE_EQ(bte.size(), i * 4 + 3);

    {
      TransitEvent* te4 = bte.back();
      REQUIRE(te4);
      te4->structured_kvs.clear();
      te4->structured_kvs.emplace_back(std::string{"test4"} + std::to_string(i), "");
      bte.push_back();
    }

    REQUIRE_EQ(bte.size(), i * 4 + 4);
  }

  for (size_t i = 0; i < 128; ++i)
  {
    // read
    {
      TransitEvent* te1 = bte.front();
      REQUIRE(te1);
      std::string const expected = std::string{"test1"} + std::to_string(i);
      REQUIRE_STREQ(te1->structured_kvs[0].first.data(), expected.data());
      bte.pop_front();
    }

    REQUIRE_EQ(bte.size(), (128 - i) * 4 - 1);

    {
      TransitEvent* te2 = bte.front();
      REQUIRE(te2);
      std::string const expected = std::string{"test2"} + std::to_string(i);
      REQUIRE_STREQ(te2->structured_kvs[0].first.data(), expected.data());
      bte.pop_front();
    }

    REQUIRE_EQ(bte.size(), (128 - i) * 4 - 2);

    {
      TransitEvent* te3 = bte.front();
      REQUIRE(te3);
      std::string const expected = std::string{"test3"} + std::to_string(i);
      REQUIRE_STREQ(te3->structured_kvs[0].first.data(), expected.data());
      bte.pop_front();
    }

    REQUIRE_EQ(bte.size(), (128 - i) * 4 - 3);

    {
      TransitEvent* te4 = bte.front();
      REQUIRE(te4);
      std::string const expected = std::string{"test4"} + std::to_string(i);
      REQUIRE_STREQ(te4->structured_kvs[0].first.data(), expected.data());
      bte.pop_front();
    }

    REQUIRE_EQ(bte.size(), (128 - i) * 4 - 4);
  }

  REQUIRE_FALSE(bte.front());
  REQUIRE(bte.empty());
  REQUIRE_EQ(bte.size(), 0);
}
