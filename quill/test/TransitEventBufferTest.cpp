#include "doctest/doctest.h"

#include "misc/DocTestExtensions.h"
#include "quill/QuillError.h"
#include "quill/detail/backend/TransitEventBuffer.h"

TEST_SUITE_BEGIN("TransitEventBuffer");

using namespace quill::detail;
using namespace quill;

/***/
TEST_CASE("transit_event_buffer_push_pop")
{
  TransitEventBuffer teb{4};

  // Nothing to read
  REQUIRE_EQ(teb.size(), 0);
  REQUIRE_EQ(teb.capacity(), 4);

  REQUIRE_EQ(teb.front(), nullptr);

  for (uint32_t i = 0; i < 10; ++i)
  {
    // push two events
    {
      REQUIRE_EQ(teb.full(), false);
      TransitEvent* next_event = teb.prepare_push();
      next_event->thread_id = std::to_string(i);
      teb.push();
    }

    {
      REQUIRE_EQ(teb.full(), false);
      TransitEvent* next_event = teb.prepare_push();
      next_event->thread_id = std::to_string(i * 2);
      teb.push();
    }

    // pop two events
    {
      REQUIRE_EQ(teb.empty(), false);
      TransitEvent* next_event = teb.front();
      REQUIRE_EQ(next_event->thread_id, std::to_string(i));
      teb.pop();
    }

    {
      REQUIRE_EQ(teb.empty(), false);
      TransitEvent* next_event = teb.front();
      REQUIRE_EQ(next_event->thread_id, std::to_string(i * 2));
      teb.pop();
    }
  }

  REQUIRE_EQ(teb.empty(), true);
  REQUIRE_EQ(teb.front(), nullptr);
}

/***/
TEST_CASE("transit_event_buffer_full")
{
  TransitEventBuffer teb{4};

  // Nothing to read
  REQUIRE_EQ(teb.size(), 0);

  REQUIRE_EQ(teb.front(), nullptr);

  for (uint32_t i = 0; i < 10; ++i)
  {
    // push three events
    {
      TransitEvent* next_event = teb.prepare_push();
      next_event->thread_id = std::to_string(i);
      teb.push();
    }

    {
      TransitEvent* next_event = teb.prepare_push();
      next_event->thread_id = std::to_string(i * 2);
      teb.push();
    }

    {
      TransitEvent* next_event = teb.prepare_push();
      next_event->thread_id = std::to_string(i * 3);
      teb.push();
    }

    // buffer full
    REQUIRE_EQ(teb.prepare_push(), nullptr);
    REQUIRE_EQ(teb.full(), true);

    // pop two events
    {
      TransitEvent* next_event = teb.front();
      REQUIRE_EQ(next_event->thread_id, std::to_string(i));
      teb.pop();
    }

    {
      TransitEvent* next_event = teb.front();
      REQUIRE_EQ(next_event->thread_id, std::to_string(i * 2));
      teb.pop();
    }

    {
      TransitEvent* next_event = teb.front();
      REQUIRE_EQ(next_event->thread_id, std::to_string(i * 3));
      teb.pop();
    }

    // buffer empty
    REQUIRE_EQ(teb.empty(), true);
    REQUIRE_EQ(teb.front(), nullptr);
  }

  REQUIRE_EQ(teb.empty(), true);
  REQUIRE_EQ(teb.front(), nullptr);
}

/***/
TEST_CASE("unbounded_transit_event_buffer")
{
  UnboundedTransitEventBuffer teb{4};

  for (uint32_t i = 0; i < 128; ++i)
  {
    // push three events
    {
      TransitEvent* next_event = teb.prepare_push();
      next_event->thread_id = std::to_string(i);
      teb.push();
    }
  }

  for (uint32_t i = 0; i < 128; ++i)
  {
    // pop two events
    {
      TransitEvent* next_event = teb.front();
      REQUIRE_EQ(next_event->thread_id, std::to_string(i));
      teb.pop();
    }
  }

  REQUIRE_EQ(teb.front(), nullptr);
}

TEST_SUITE_END();