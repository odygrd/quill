#include "doctest/doctest.h"

#include "misc/DocTestExtensions.h"
#include "quill/detail/BacktraceLogRecordStorage.h"
#include "quill/detail/events/BaseEvent.h"
#include <iostream>

TEST_SUITE_BEGIN("BacktraceLogRecordStorage");

using namespace quill;
using namespace quill::detail;

class TestRecord final : public BaseEvent
{
public:
  explicit TestRecord(uint32_t x, uint32_t y) : x(x), y(y) {}

  std::unique_ptr<BaseEvent> clone() const override { return std::make_unique<TestRecord>(*this); }

  size_t size() const noexcept override { return sizeof(*this); }

  void backend_process(BacktraceLogRecordStorage&, char const*, GetHandlersCallbackT const&,
                       GetRealTsCallbackT const&) const noexcept override
  {
  }

  /**
   * Destructor
   */
  ~TestRecord() override = default;

  uint32_t x{0};
  uint32_t y{0};
};

bool operator==(TestRecord const& lhs, TestRecord const& rhs)
{
  return lhs.x == rhs.x && lhs.y == rhs.y;
}

std::ostream& operator<<(std::ostream& os, TestRecord const& obj)
{
  os << "x: " << obj.x << " y: " << obj.y;
  return os;
}

TEST_CASE("retrieve_records_when_empty")
{
  constexpr uint32_t max_messages{4};
  char const* logger_name = "logger1";

  BacktraceLogRecordStorage backtrace_log_record_storage;
  backtrace_log_record_storage.set_capacity(logger_name, max_messages);

  // Check retrieved records - nothing to process
  uint32_t counter = 0;
  backtrace_log_record_storage.process(
    logger_name, [&counter](std::string const& thread_id, BaseEvent const*) { ++counter; });
  REQUIRE_EQ(counter, 0);
}

TEST_CASE("store_clear_records")
{
  constexpr uint32_t max_messages{4};
  char const* logger_name = "logger1";
  char const* thread_id = "123";

  BacktraceLogRecordStorage backtrace_log_record_storage;
  backtrace_log_record_storage.set_capacity(logger_name, max_messages);

  // Insert 2 Records
  for (uint32_t i = 0; i < 2; ++i)
  {
    TestRecord test_record{i, i * 10};
    backtrace_log_record_storage.store(logger_name, thread_id, test_record.clone());
  }

  // Clear them
  backtrace_log_record_storage.clear(logger_name);

  // Check retrieved records - nothing to process
  uint32_t counter = 0;
  backtrace_log_record_storage.process(
    logger_name, [&counter](std::string const& thread_id, BaseEvent const*) { ++counter; });
  REQUIRE_EQ(counter, 0);
}

TEST_CASE("store_then_change_capacity")
{
  constexpr uint32_t max_messages{4};
  char const* logger_name = "logger1";
  char const* thread_id = "123";

  BacktraceLogRecordStorage backtrace_log_record_storage;
  backtrace_log_record_storage.set_capacity(logger_name, max_messages);

  // Insert 2 Records
  for (uint32_t i = 0; i < 2; ++i)
  {
    TestRecord test_record{i, i * 10};
    backtrace_log_record_storage.store(logger_name, thread_id, test_record.clone());
  }

  // Set a different capacity
  backtrace_log_record_storage.set_capacity(logger_name, max_messages * 2);

  // Check retrieved records - nothing to process - all was cleared because we reset capacity
  uint32_t counter = 0;
  backtrace_log_record_storage.process(
    logger_name, [&counter](std::string const& thread_id, BaseEvent const*) { ++counter; });
  REQUIRE_EQ(counter, 0);
}

TEST_CASE("store_then_set_same_capacity")
{
  constexpr uint32_t max_messages{4};
  char const* logger_name = "logger1";
  char const* thread_id = "123";

  BacktraceLogRecordStorage backtrace_log_record_storage;
  backtrace_log_record_storage.set_capacity(logger_name, max_messages);

  // Insert 2 Records
  for (uint32_t i = 0; i < 2; ++i)
  {
    TestRecord test_record{i, i * 10};
    backtrace_log_record_storage.store(logger_name, thread_id, test_record.clone());
  }

  // Set a different capacity
  backtrace_log_record_storage.set_capacity(logger_name, max_messages);

  // Check retrieved records - still two to process as we set the same capacity the messages didn't reset
  uint32_t counter = 0;
  backtrace_log_record_storage.process(
    logger_name, [&counter](std::string const& thread_id, BaseEvent const*) { ++counter; });
  REQUIRE_EQ(counter, 2);
}

TEST_CASE("store_then_process_no_wrap_around")
{
  constexpr uint32_t max_messages{4};
  char const* logger_name = "logger1";
  char const* thread_id = "123";

  BacktraceLogRecordStorage backtrace_log_record_storage;
  backtrace_log_record_storage.set_capacity(logger_name, max_messages);

  // Insert 2 Records
  for (uint32_t i = 0; i < 2; ++i)
  {
    TestRecord test_record{i, i * 10};
    backtrace_log_record_storage.store(logger_name, thread_id, test_record.clone());
  }

  // Check retrieved records
  uint32_t counter = 0;
  backtrace_log_record_storage.process(
    logger_name, [&counter](std::string const& retrieved_thread_id, BaseEvent const* retrieved_record) {
      TestRecord expected_record{counter, counter * 10};
      REQUIRE_STREQ(retrieved_thread_id.data(), "123");
      REQUIRE_EQ(*(reinterpret_cast<TestRecord const*>(retrieved_record)), expected_record);
      ++counter;
    });
  REQUIRE_EQ(counter, 2);

  // Check no more retrieved records
  counter = 0;
  backtrace_log_record_storage.process(
    logger_name, [&counter](std::string const& thread_id, BaseEvent const*) { ++counter; });
  REQUIRE_EQ(counter, 0);
}

TEST_CASE("store_then_process_wrap_around")
{
  constexpr uint32_t max_messages{5};
  char const* logger_name = "logger1";
  char const* thread_id = "123";

  BacktraceLogRecordStorage backtrace_log_record_storage;
  backtrace_log_record_storage.set_capacity(logger_name, max_messages);

  // Insert 12 Records
  for (uint32_t i = 0; i < 12; ++i)
  {
    TestRecord test_record{i, i * 10};
    backtrace_log_record_storage.store(logger_name, thread_id, test_record.clone());
  }

  // Check retrieved records
  uint32_t counter = 0;
  backtrace_log_record_storage.process(
    logger_name, [&counter](std::string const& retrieved_thread_id, BaseEvent const* retrieved_record) {
      TestRecord expected_record{counter + 7, (counter + 7) * 10};
      REQUIRE_STREQ(retrieved_thread_id.data(), "123");
      REQUIRE_EQ(*(reinterpret_cast<TestRecord const*>(retrieved_record)), expected_record);
      ++counter;
    });
  REQUIRE_EQ(counter, max_messages);
}

TEST_SUITE_END();