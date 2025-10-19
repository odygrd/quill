#include "DocTestExtensions.h"
#include "doctest/doctest.h"

#include "quill/backend/TransitEventBuffer.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>

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

/***/
TEST_CASE("transit_event_large_format_buffer_with_reallocations")
{
  // Test with FormatBuffer > 128 bytes (TransitEvent::FormatBuffer stack size is 88)
  // Use random sizes [20, 512] to trigger various reallocation scenarios
  TransitEventBuffer bte{4};

  std::vector<std::pair<size_t, std::string>> expected_data;
  std::srand(static_cast<unsigned>(std::time(nullptr)));

  // Generate and store events with varying buffer sizes, triggering multiple reallocations
  for (size_t iteration = 0; iteration < 10000; ++iteration)
  {
    size_t buffer_size = 20 + (std::rand() % 493); // Random size [20, 512]
    std::string large_string(buffer_size, 'A' + static_cast<char>(iteration % 26));

    TransitEvent* te = bte.back();
    REQUIRE(te);

    te->formatted_msg->clear();
    te->formatted_msg->append(large_string.data(), large_string.data() + large_string.size());

    // Also store named args with iteration number for validation
    te->extra_data = std::make_unique<TransitEvent::ExtraData>();
    te->get_named_args()->clear();
    te->get_named_args()->emplace_back("iteration_" + std::to_string(iteration), std::to_string(buffer_size));

    expected_data.emplace_back(buffer_size, large_string);

    // Call back multiple times for additional testing
    for (size_t j = 0; j < 3; ++j)
    {
      // Pull same record and verify again
      TransitEvent* te2 = bte.back();
      REQUIRE(te2);
      std::string_view actual_msg(te2->formatted_msg->data(), te2->formatted_msg->size());
      REQUIRE_EQ(actual_msg, large_string);
    }

    bte.push_back();

    // Verify the buffer is handling reallocation correctly
    REQUIRE_EQ(bte.size(), iteration + 1);
  }

  // Verify all data integrity after multiple reallocations
  for (size_t i = 0; i < expected_data.size(); ++i)
  {
    // Call front multiple times for additional testing
    for (size_t j = 0; j < 3; ++j)
    {
      TransitEvent* te = bte.front();
      REQUIRE(te);

      // Check formatted_msg content and size
      REQUIRE_EQ(te->formatted_msg->size(), expected_data[i].first);
      std::string_view actual_msg(te->formatted_msg->data(), te->formatted_msg->size());
      REQUIRE_EQ(actual_msg, expected_data[i].second);

      // Verify named_args are intact
      std::string expected_key = "iteration_" + std::to_string(i);
      std::string expected_value = std::to_string(expected_data[i].first);
      REQUIRE_STREQ((*te->get_named_args())[0].first.data(), expected_key.data());
      REQUIRE_STREQ((*te->get_named_args())[0].second.data(), expected_value.data());
    }

    bte.pop_front();
  }

  REQUIRE(bte.empty());
}

/***/
TEST_CASE("transit_event_buffer_shrink_and_regrow")
{
  // Test shrinking buffer and then growing again to detect memory issues
  TransitEventBuffer bte{4};

  // Phase 1: Fill buffer to trigger expansion
  std::vector<std::string> phase1_data;
  for (size_t i = 0; i < 32; ++i)
  {
    std::string data = "phase1_" + std::to_string(i);
    phase1_data.push_back(data);

    TransitEvent* te = bte.back();
    REQUIRE(te);

    te->formatted_msg->clear();
    te->formatted_msg->append(data.data(), data.data() + data.size());

    te->extra_data = std::make_unique<TransitEvent::ExtraData>();
    te->get_named_args()->clear();
    te->get_named_args()->emplace_back(data, std::to_string(i));

    bte.push_back();
  }

  size_t expanded_capacity = bte.capacity();
  REQUIRE_GT(expanded_capacity, 4);
  REQUIRE_EQ(bte.size(), 32);

  // Phase 2: Empty the buffer
  for (size_t i = 0; i < phase1_data.size(); ++i)
  {
    TransitEvent* te = bte.front();
    REQUIRE(te);
    std::string_view msg(te->formatted_msg->data(), te->formatted_msg->size());
    REQUIRE_EQ(msg, phase1_data[i]);
    bte.pop_front();
  }

  REQUIRE(bte.empty());

  // Phase 3: Request shrink and verify
  bte.request_shrink();
  bte.try_shrink();
  REQUIRE_EQ(bte.capacity(), 4); // Should shrink back to initial

  // Phase 4: Regrow with large buffers and verify data integrity
  std::vector<std::pair<std::string, size_t>> phase4_data;
  for (size_t i = 0; i < 50; ++i)
  {
    size_t buffer_size = 100 + (i * 7); // Varying sizes starting from 100
    std::string large_data(buffer_size, 'Z' - static_cast<char>(i % 26));
    phase4_data.emplace_back(large_data, buffer_size);

    TransitEvent* te = bte.back();
    REQUIRE(te);

    te->formatted_msg->clear();
    te->formatted_msg->append(large_data.data(), large_data.data() + large_data.size());

    te->extra_data = std::make_unique<TransitEvent::ExtraData>();
    te->get_named_args()->clear();
    te->get_named_args()->emplace_back("regrow_" + std::to_string(i), std::to_string(buffer_size));

    bte.push_back();
  }

  REQUIRE_GT(bte.capacity(), 4); // Should have expanded again
  REQUIRE_EQ(bte.size(), 50);

  // Phase 5: Verify all data after shrink/regrow cycle
  for (size_t i = 0; i < phase4_data.size(); ++i)
  {
    TransitEvent* te = bte.front();
    REQUIRE(te);

    REQUIRE_EQ(te->formatted_msg->size(), phase4_data[i].second);
    std::string_view actual(te->formatted_msg->data(), te->formatted_msg->size());
    REQUIRE_EQ(actual, phase4_data[i].first);

    std::string expected_key = "regrow_" + std::to_string(i);
    REQUIRE_STREQ((*te->get_named_args())[0].first.data(), expected_key.data());

    bte.pop_front();
  }

  REQUIRE(bte.empty());
}

/***/
TEST_CASE("transit_event_buffer_interleaved_operations")
{
  // Test interleaving push/pop operations with varying buffer sizes to stress test reallocation
  TransitEventBuffer bte{2};

  std::vector<std::pair<std::string, size_t>> active_data;
  size_t total_operations = 0;

  std::srand(static_cast<unsigned>(std::time(nullptr)) + 42);

  for (size_t cycle = 0; cycle < 20; ++cycle)
  {
    // Push phase: Add 10-20 events with varying sizes
    size_t push_count = 10 + (std::rand() % 11);
    for (size_t i = 0; i < push_count; ++i)
    {
      size_t buffer_size = 50 + (std::rand() % 400); // [50, 449]
      std::string data(buffer_size, 'a' + static_cast<char>((total_operations + i) % 26));
      std::string key = "cycle" + std::to_string(cycle) + "_item" + std::to_string(i);

      TransitEvent* te = bte.back();
      REQUIRE(te);

      // Test formatted_msg with heap-allocated buffer
      te->formatted_msg->clear();
      te->formatted_msg->append(data.data(), data.data() + data.size());

      // Test extra_data with named_args
      te->extra_data = std::make_unique<TransitEvent::ExtraData>();
      te->get_named_args()->clear();
      te->get_named_args()->emplace_back(key, std::to_string(buffer_size));

      active_data.emplace_back(data, buffer_size);
      bte.push_back();
    }

    total_operations += push_count;
    REQUIRE_EQ(bte.size(), active_data.size());

    // Pop phase: Remove 5-15 events and verify their content
    size_t pop_count = std::min(static_cast<size_t>(5 + (std::rand() % 11)), active_data.size());
    for (size_t i = 0; i < pop_count; ++i)
    {
      TransitEvent* te = bte.front();
      REQUIRE(te);

      // Verify formatted_msg content matches
      REQUIRE_EQ(te->formatted_msg->size(), active_data[0].second);
      std::string_view actual_msg(te->formatted_msg->data(), te->formatted_msg->size());
      REQUIRE_EQ(actual_msg, active_data[0].first);

      // Verify named_args are valid
      REQUIRE(te->get_named_args());
      REQUIRE_EQ(te->get_named_args()->size(), 1);
      REQUIRE_EQ((*te->get_named_args())[0].second, std::to_string(active_data[0].second));

      bte.pop_front();
      active_data.erase(active_data.begin());
    }

    REQUIRE_EQ(bte.size(), active_data.size());
  }

  // Final verification: drain remaining events
  while (!active_data.empty())
  {
    TransitEvent* te = bte.front();
    REQUIRE(te);

    REQUIRE_EQ(te->formatted_msg->size(), active_data[0].second);
    std::string_view actual_msg(te->formatted_msg->data(), te->formatted_msg->size());
    REQUIRE_EQ(actual_msg, active_data[0].first);

    bte.pop_front();
    active_data.erase(active_data.begin());
  }

  REQUIRE(bte.empty());
  REQUIRE_EQ(bte.size(), 0);
}

/***/
TEST_CASE("transit_event_buffer_shrink_flag_with_move_semantics")
{
  // Test that _shrink_requested flag is properly handled during move operations
  TransitEventBuffer bte1{4};

  // Fill and then empty the buffer
  for (size_t i = 0; i < 10; ++i)
  {
    TransitEvent* te = bte1.back();
    REQUIRE(te);
    std::string data = "test_" + std::to_string(i);
    te->formatted_msg->clear();
    te->formatted_msg->append(data.data(), data.data() + data.size());
    bte1.push_back();
  }

  REQUIRE_GT(bte1.capacity(), 4);

  // Drain the buffer
  while (!bte1.empty())
  {
    bte1.pop_front();
  }

  // Request shrink
  bte1.request_shrink();

  // Move construct a new buffer
  TransitEventBuffer bte2{std::move(bte1)};

  // The shrink request should be preserved in bte2
  // Try to shrink bte2
  bte2.try_shrink();

  REQUIRE_EQ(bte2.capacity(), 4);
}

/***/
TEST_CASE("transit_event_copy_to_backtrace_scenario")
{
  // This simulates the backtrace scenario in BackendWorker where copy_to() is used
  // See BackendWorker.h:851-852
  TransitEventBuffer bte{4};

  // Create a complex transit event
  TransitEvent* te1 = bte.back();
  REQUIRE(te1);

  std::string large_msg(300, 'A');
  te1->formatted_msg->clear();
  te1->formatted_msg->append(large_msg.data(), large_msg.data() + large_msg.size());

  te1->extra_data = std::make_unique<TransitEvent::ExtraData>();
  te1->get_named_args()->emplace_back("key1", "value1");
  te1->get_named_args()->emplace_back("key2", "value2");

  bte.push_back();

  // Now simulate the backtrace copy scenario
  TransitEvent* original = bte.front();
  REQUIRE(original);

  TransitEvent copy_event;
  original->copy_to(copy_event);

  // Verify the copy is independent and correct
  REQUIRE(copy_event.formatted_msg);
  REQUIRE_EQ(copy_event.formatted_msg->size(), 300);
  REQUIRE_EQ(std::string_view(copy_event.formatted_msg->data(), copy_event.formatted_msg->size()), large_msg);

  REQUIRE(copy_event.extra_data);
  REQUIRE_EQ(copy_event.get_named_args()->size(), 2);
  REQUIRE_EQ((*copy_event.get_named_args())[0].first, "key1");
  REQUIRE_EQ((*copy_event.get_named_args())[1].second, "value2");

  // Modify original and verify copy is unaffected
  std::string modified = "MODIFIED";
  original->formatted_msg->clear();
  original->formatted_msg->append(modified.data(), modified.data() + modified.size());

  REQUIRE_EQ(std::string_view(copy_event.formatted_msg->data(), copy_event.formatted_msg->size()), large_msg);

  bte.pop_front();
  REQUIRE(bte.empty());
}

/***/
TEST_CASE("transit_event_formatted_msg_validity_after_expansion")
{
  // Test that formatted_msg unique_ptr remains valid after buffer expansion
  // This tests for potential issues with move semantics and reallocation
  TransitEventBuffer bte{2};

  // Fill buffer to capacity to trigger expansion on next back()
  for (size_t i = 0; i < 2; ++i)
  {
    TransitEvent* te = bte.back();
    REQUIRE(te);
    REQUIRE(te->formatted_msg); // Should always be valid

    std::string data = "test_" + std::to_string(i);
    te->formatted_msg->clear();
    te->formatted_msg->append(data.data(), data.data() + data.size());

    bte.push_back();
  }

  REQUIRE_EQ(bte.size(), 2);
  REQUIRE_EQ(bte.capacity(), 2);

  // Next back() call will trigger expansion
  TransitEvent* te_new = bte.back();
  REQUIRE(te_new);
  REQUIRE(te_new->formatted_msg);

  // Verify we can use it safely
  te_new->formatted_msg->clear();
  std::string large_data(200, 'X');
  te_new->formatted_msg->append(large_data.data(), large_data.data() + large_data.size());

  bte.push_back();

  // Verify capacity increased
  REQUIRE_GT(bte.capacity(), 2);
  REQUIRE_EQ(bte.size(), 3);

  // Verify all events are still accessible with valid formatted_msg
  for (size_t i = 0; i < 3; ++i)
  {
    TransitEvent* te = bte.front();
    REQUIRE(te);
    REQUIRE(te->formatted_msg); // Must still be valid
    REQUIRE_GT(te->formatted_msg->size(), 0);
    bte.pop_front();
  }

  REQUIRE(bte.empty());
}

/***/
TEST_CASE("transit_event_buffer_mask_consistency_during_expansion")
{
  TransitEventBuffer bte{4};

  // Fill completely
  for (size_t i = 0; i < 4; ++i)
  {
    TransitEvent* te = bte.back();
    te->timestamp = i * 100;
    te->logger_base = reinterpret_cast<detail::LoggerBase*>(static_cast<uintptr_t>(i + 1));
    bte.push_back();
  }

  // Pop 2 to create wraparound scenario
  bte.pop_front();
  bte.pop_front();

  // Add 2 more - these will use positions 0 and 1 (wraparound)
  for (size_t i = 0; i < 2; ++i)
  {
    TransitEvent* te = bte.back();
    te->timestamp = (i + 10) * 100;
    te->logger_base = reinterpret_cast<detail::LoggerBase*>(static_cast<uintptr_t>(i + 10));
    bte.push_back();
  }

  // Now buffer is full again: positions [0,1,2,3] with reader at 2
  REQUIRE_EQ(bte.size(), 4);
  REQUIRE_EQ(bte.capacity(), 4);

  // Next back() will expand
  TransitEvent* te_expand = bte.back();
  REQUIRE(te_expand);

  // After expansion, verify all existing events are still accessible with correct data
  REQUIRE_EQ(bte.capacity(), 8);

  // Set the new event
  te_expand->timestamp = 2000;
  te_expand->logger_base = reinterpret_cast<detail::LoggerBase*>(static_cast<uintptr_t>(99));
  bte.push_back();

  // Verify all 5 events
  std::vector<uint64_t> expected_timestamps = {200, 300, 1000, 1100, 2000};
  for (auto const expected_timestamp : expected_timestamps)
  {
    TransitEvent* te = bte.front();
    REQUIRE(te);
    REQUIRE_EQ(te->timestamp, expected_timestamp);
    REQUIRE(te->logger_base);
    bte.pop_front();
  }

  REQUIRE(bte.empty());
}

/***/
TEST_CASE("transit_event_buffer_stress_test")
{
  TransitEventBuffer bte{4};

  size_t constexpr total_events = 200;

  std::srand(static_cast<unsigned>(std::time(nullptr)) + 12345);

  size_t global_event_id = 0;

  for (size_t iteration = 0; iteration < 64; ++iteration)
  {
    // Phase 1: Push total_events (will trigger multiple reallocations)
    for (size_t push_idx = 0; push_idx < total_events; ++push_idx)
    {
      TransitEvent* te = bte.back();
      REQUIRE(te);
      REQUIRE(te->formatted_msg);

      // Populate with large formatted_msg (20 to 8192 bytes) to stress memory
      size_t msg_size = 20 + (std::rand() % 8173);
      te->formatted_msg->clear();
      te->formatted_msg->reserve(msg_size);

      // Fill with recognizable pattern
      char pattern_char = 'A' + (global_event_id % 26);
      for (size_t i = 0; i < msg_size; ++i)
      {
        te->formatted_msg->push_back(pattern_char);
      }
      te->timestamp = global_event_id;

      REQUIRE_EQ(te->formatted_msg->size(), msg_size);

      bte.push_back();
      global_event_id++;
    }

    REQUIRE_EQ(bte.size(), total_events);

    // Phase 2: Pop all total_events and verify
    for (size_t pop_idx = 0; pop_idx < total_events; ++pop_idx)
    {
      TransitEvent* te = bte.front();
      REQUIRE(te);
      REQUIRE(te->formatted_msg);

      size_t expected_id = (iteration * total_events) + pop_idx;

      REQUIRE_EQ(te->timestamp, expected_id);

      // Verify formatted_msg pattern
      char expected_char = 'A' + (expected_id % 26);
      for (size_t i = 0; i < te->formatted_msg->size(); ++i)
      {
        REQUIRE_EQ((*te->formatted_msg)[i], expected_char);
      }

      bte.pop_front();
    }

    REQUIRE(bte.empty());
    REQUIRE_EQ(bte.size(), 0);

    // Phase 3: Shrink every 5 iterations
    if ((iteration + 1) % 5 == 0)
    {
      bte.request_shrink();
      bte.try_shrink();
      REQUIRE_EQ(bte.capacity(), 4); // Should shrink back to initial capacity
    }
  }

  REQUIRE(bte.empty());
  REQUIRE_EQ(global_event_id, 12800);
}

TEST_SUITE_END();
