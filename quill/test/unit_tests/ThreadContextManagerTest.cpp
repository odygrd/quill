#include "doctest/doctest.h"

#include "quill/core/FrontendOptions.h"
#include "quill/core/ThreadContextManager.h"
#include <array>
#include <thread>

TEST_SUITE_BEGIN("ThreadContextManager");

using namespace quill;
using namespace quill::detail;

/***/
TEST_CASE("add_and_remove_thread_contexts")
{
  // 1) Test that every time a new thread context is added to the thread context shared collection
  // and to the thread context cache when a new thread spawns and we load the cache
  // 2) Test that the thread context is invalidated when the thread that created it completes
  // 3) Test that when the threads complete they are removed

  constexpr uint32_t tries = 4;
  for (uint32_t k = 0; k < tries; ++k)
  {
    constexpr size_t num_threads{25};
    std::array<std::thread, num_threads> threads;
    std::array<std::atomic<bool>, num_threads> terminate_flag{};
    std::fill(terminate_flag.begin(), terminate_flag.end(), false);

    std::atomic<uint32_t> threads_started{0};

    // spawn x number of threads
    for (size_t i = 0; i < threads.size(); ++i)
    {
      auto& thread_terminate_flag = terminate_flag[i];
      threads[i] = std::thread(
        [&thread_terminate_flag, &threads_started]()
        {
          // create a context for that thread
          ThreadContext* tc = get_local_thread_context<FrontendOptions>();

          REQUIRE(tc->has_unbounded_queue_type());
          REQUIRE(tc->has_blocking_queue());

          REQUIRE_FALSE(tc->has_bounded_queue_type());
          REQUIRE_FALSE(tc->has_dropping_queue());

          threads_started.fetch_add(1);
          while (!thread_terminate_flag.load())
          {
            // loop waiting for main to signal
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
          }
        });
    }

    // main wait for all of them to start
    while (threads_started.load() < num_threads)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }

    // Check we have exactly as many thread contexts as the amount of threads in our backend cache
    uint64_t thread_ctx_cnt{0};

    // Check all thread contexts in the backend thread contexts cache
    ThreadContextManager::instance().for_each_thread_context(
      [&thread_ctx_cnt](ThreadContext const* tc)
      {
        REQUIRE(tc->is_valid_context());
        REQUIRE(tc->get_spsc_queue<FrontendOptions::queue_type>().empty());
        ++thread_ctx_cnt;
      });

    REQUIRE_EQ(thread_ctx_cnt, num_threads);
    REQUIRE_FALSE(ThreadContextManager::instance().has_invalid_thread_context());

    // terminate all threads - This will invalidate all the thread contexts
    for (size_t j = 0; j < threads.size(); ++j)
    {
      terminate_flag[j].store(true);
      threads[j].join();
    }

    REQUIRE(ThreadContextManager::instance().has_invalid_thread_context());

    // Now check all thread contexts still exist but they are invalided and then remove them

    REQUIRE(ThreadContextManager::instance().has_invalid_thread_context());

    // For this we use the old cache avoiding to update it - This never happens in the real logger
    std::vector<ThreadContext const*> tc_cache;
    ThreadContextManager::instance().for_each_thread_context(
      [&tc_cache](ThreadContext const* tc)
      {
        REQUIRE_FALSE(tc->is_valid_context());
        REQUIRE(tc->get_spsc_queue<FrontendOptions::queue_type>().empty());
        tc_cache.push_back(tc);
      });

    // Remove them
    for (ThreadContext const* tc : tc_cache)
    {
      ThreadContextManager::instance().remove_shared_invalidated_thread_context(tc);
    }

    // Check all are removed
    bool tc_found = false;
    ThreadContextManager::instance().for_each_thread_context([&tc_found](ThreadContext const*)
                                                             { tc_found = true; });

    REQUIRE_FALSE(tc_found);
  }
}

TEST_SUITE_END();