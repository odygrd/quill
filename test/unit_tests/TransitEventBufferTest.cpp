#include "DocTestExtensions.h"
#include "doctest/doctest.h"

#include "quill/backend/TransitEventBuffer.h"

TEST_SUITE_BEGIN("TransitEventBuffer");

using namespace quill;
using namespace quill::detail;

/***/
TEST_CASE("transit_event_unbounded_buffer")
{
  TransitEventBuffer bte{4};

  REQUIRE_FALSE(bte.front());
  REQUIRE(bte.empty());
  REQUIRE_EQ(bte.size(), 0);

  for (size_t i = 0; i < 128; ++i)
  {
    {
      TransitEvent* te1 = bte.back();
      REQUIRE(te1);

      te1->extra_data = std::make_unique<TransitEvent::ExtraData>();
      te1->get_named_args()->clear();
      te1->get_named_args()->emplace_back(std::string{"test1"} + std::to_string(i), std::string{});
      bte.push_back();
    }

    REQUIRE_EQ(bte.size(), i * 4 + 1);

    {
      TransitEvent* te2 = bte.back();
      REQUIRE(te2);

      te2->extra_data = std::make_unique<TransitEvent::ExtraData>();
      te2->get_named_args()->clear();
      te2->get_named_args()->emplace_back(std::string{"test2"} + std::to_string(i), std::string{});
      bte.push_back();
    }

    REQUIRE_EQ(bte.size(), i * 4 + 2);

    {
      TransitEvent* te3 = bte.back();
      REQUIRE(te3);

      te3->extra_data = std::make_unique<TransitEvent::ExtraData>();
      te3->get_named_args()->clear();
      te3->get_named_args()->emplace_back(std::string{"test3"} + std::to_string(i), std::string{});
      bte.push_back();
    }

    REQUIRE_EQ(bte.size(), i * 4 + 3);

    {
      TransitEvent* te4 = bte.back();
      REQUIRE(te4);

      te4->extra_data = std::make_unique<TransitEvent::ExtraData>();
      te4->get_named_args()->clear();
      te4->get_named_args()->emplace_back(std::string{"test4"} + std::to_string(i), std::string{});
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
      REQUIRE_STREQ((*te1->get_named_args())[0].first.data(), expected.data());
      bte.pop_front();
    }

    REQUIRE_EQ(bte.size(), (128 - i) * 4 - 1);

    {
      TransitEvent* te2 = bte.front();
      REQUIRE(te2);
      std::string const expected = std::string{"test2"} + std::to_string(i);
      REQUIRE_STREQ((*te2->get_named_args())[0].first.data(), expected.data());
      bte.pop_front();
    }

    REQUIRE_EQ(bte.size(), (128 - i) * 4 - 2);

    {
      TransitEvent* te3 = bte.front();
      REQUIRE(te3);
      std::string const expected = std::string{"test3"} + std::to_string(i);
      REQUIRE_STREQ((*te3->get_named_args())[0].first.data(), expected.data());
      bte.pop_front();
    }

    REQUIRE_EQ(bte.size(), (128 - i) * 4 - 3);

    {
      TransitEvent* te4 = bte.front();
      REQUIRE(te4);
      std::string const expected = std::string{"test4"} + std::to_string(i);
      REQUIRE_STREQ((*te4->get_named_args())[0].first.data(), expected.data());
      bte.pop_front();
    }

    REQUIRE_EQ(bte.size(), (128 - i) * 4 - 4);
  }

  REQUIRE_FALSE(bte.front());
  REQUIRE(bte.empty());
  REQUIRE_EQ(bte.size(), 0);
}
