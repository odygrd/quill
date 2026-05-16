#include "doctest/doctest.h"

#include "quill/backend/BacktraceStorage.h"

TEST_SUITE_BEGIN("BacktraceStorage");

using namespace quill::detail;

TEST_CASE("store_noops_when_capacity_is_zero")
{
  BacktraceStorage backtrace_storage;

  backtrace_storage.store(TransitEvent{}, "thread_1", "worker");

  size_t callback_invocations = 0;
  backtrace_storage.process([&callback_invocations](TransitEvent const&, std::string_view, std::string_view)
                            { ++callback_invocations; });

  REQUIRE_EQ(callback_invocations, 0);
}

TEST_CASE("set_capacity_zero_clears_existing_storage")
{
  BacktraceStorage backtrace_storage;
  backtrace_storage.set_capacity(2);

  backtrace_storage.store(TransitEvent{}, "thread_1", "worker");
  backtrace_storage.store(TransitEvent{}, "thread_2", "worker");

  backtrace_storage.set_capacity(0);
  backtrace_storage.store(TransitEvent{}, "thread_3", "worker");

  size_t callback_invocations = 0;
  backtrace_storage.process([&callback_invocations](TransitEvent const&, std::string_view, std::string_view)
                            { ++callback_invocations; });

  REQUIRE_EQ(callback_invocations, 0);
}

TEST_SUITE_END();
