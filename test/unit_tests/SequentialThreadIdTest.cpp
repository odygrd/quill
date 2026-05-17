#define QUILL_USE_SEQUENTIAL_THREAD_ID

#include "quill/Utility.h"

QUILL_DEFINE_SEQUENTIAL_THREAD_ID

#include "doctest/doctest.h"

#include "quill/backend/ThreadUtilities.h"
#include "quill/core/FrontendOptions.h"
#include "quill/core/ThreadContextManager.h"

#include <atomic>
#include <string>
#include <thread>

TEST_SUITE_BEGIN("SequentialThreadId");

using namespace quill;
using namespace quill::detail;

TEST_CASE("sequential_thread_id_is_stable_per_thread")
{
  uint32_t const main_thread_id = get_thread_id();
  REQUIRE_NE(main_thread_id, 0u);

  for (uint32_t i = 0; i < 3; ++i)
  {
    REQUIRE_EQ(get_thread_id(), main_thread_id);
  }

  ThreadContext* main_thread_context = get_local_thread_context<FrontendOptions>();
  REQUIRE_EQ(static_cast<uint32_t>(std::stoul(std::string{main_thread_context->thread_id()})), main_thread_id);

  std::atomic<uint32_t> worker_thread_id{0};
  std::atomic<uint32_t> worker_thread_context_id{0};
  std::atomic<bool> worker_same_tu_stable{false};

  std::thread worker(
    [&]()
    {
      uint32_t const current_thread_id = get_thread_id();
      worker_thread_id.store(current_thread_id, std::memory_order_relaxed);
      worker_same_tu_stable.store(get_thread_id() == current_thread_id, std::memory_order_relaxed);

      ThreadContext* worker_thread_context = get_local_thread_context<FrontendOptions>();
      worker_thread_context_id.store(
        static_cast<uint32_t>(std::stoul(std::string{worker_thread_context->thread_id()})), std::memory_order_relaxed);
    });

  worker.join();

  REQUIRE_NE(worker_thread_id.load(std::memory_order_relaxed), 0u);
  REQUIRE_NE(worker_thread_id.load(std::memory_order_relaxed), main_thread_id);
  REQUIRE(worker_same_tu_stable.load(std::memory_order_relaxed));
  REQUIRE_EQ(worker_thread_context_id.load(std::memory_order_relaxed),
             worker_thread_id.load(std::memory_order_relaxed));
}

TEST_SUITE_END();
