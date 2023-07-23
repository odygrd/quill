#include "doctest/doctest.h"

#include "quill/Config.h"
#include "quill/detail/ThreadContext.h"
#include "quill/detail/ThreadContextCollection.h"
#include <array>
#include <thread>

TEST_SUITE_BEGIN("ThreadContextCollection");

using namespace quill;
using namespace quill::detail;

/***/
TEST_CASE("add_remove_thread_context_multithreaded_wait_for_threads_to_join")
{
  // 1) Test that every time a new thread context is added to the thread context shared collection
  // and to the thread context cache when a new thread spawns and we load the cache
  // 2) Test that the thread context is invalidated when the thread that created it completes
  // 3) Test that the thread context cache is updated correctly and the contexts are removed from
  // the cache when the threads complete

  // run the test multiple times to create many thread contexts for the same thread context collection
  Config cfg;
  ThreadContextCollection thread_context_collection{cfg};

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
      threads[i] = std::thread([&thread_terminate_flag, &threads_started, &thread_context_collection]()
        {
          // create a context for that thread
          QUILL_MAYBE_UNUSED auto tc = thread_context_collection.local_thread_context<QUILL_QUEUE_TYPE>();
          threads_started.fetch_add(1);
          while (!thread_terminate_flag.load())
          {
            // loop waiting for main to signal
            std::this_thread::sleep_for(std::chrono::nanoseconds{10});
          }
        });
    }

    // main wait for all of them to start
    while (threads_started.load() < num_threads)
    {
      // loop until all threads start
      // Instead of just waiting we will just call get cached contexts many times for additional testing
      thread_context_collection.clear_invalid_and_empty_thread_contexts();
      QUILL_MAYBE_UNUSED auto& cached_thread_contexts =
        thread_context_collection.backend_thread_contexts_cache();
    }

    // Check we have exactly as many thread contexts as the amount of threads in our backend cache
    thread_context_collection.clear_invalid_and_empty_thread_contexts();
    REQUIRE_EQ(thread_context_collection.backend_thread_contexts_cache().size(), num_threads);

    // Check all thread contexts in the backend thread contexts cache
    thread_context_collection.clear_invalid_and_empty_thread_contexts();
    auto const backend_thread_contexts_cache_local =
      thread_context_collection.backend_thread_contexts_cache();

    for (auto& thread_context : backend_thread_contexts_cache_local)
    {
      REQUIRE(thread_context->is_valid());
      REQUIRE(thread_context->spsc_queue<QUILL_QUEUE_TYPE>().empty());
    }

    // terminate all threads - This will invalidate all the contracts
    for (size_t j = 0; j < threads.size(); ++j)
    {
      terminate_flag[j].store(true);
      threads[j].join();
    }

    // Now check all thread contexts still exist but they are invalided and then remove_file them
    // For this we use the old cache avoiding to update it - This never happens in the real logger
    for (auto* thread_context : backend_thread_contexts_cache_local)
    {
      REQUIRE_FALSE(thread_context->is_valid());
      REQUIRE(thread_context->spsc_queue<QUILL_QUEUE_TYPE>().empty());
    }

    // Check there is no thread context left by getting the updated cache via the call
    // to backend_thread_contexts_cache()
    thread_context_collection.clear_invalid_and_empty_thread_contexts();
    REQUIRE_EQ(thread_context_collection.backend_thread_contexts_cache().size(), 0);
  }
}

/***/
TEST_CASE("add_remove_thread_context_multithreaded_dont_wait_for_threads_to_join")
{
  // 1) Test that every time a new thread context is added to the thread context shared collection
  // and to the thread context cache when a new thread spawns and we load the cache
  // 2) Test that the thread context is invalidated when the thread that created it completes
  // 3) Test that the thread context cache is updated correctly and the contexts are removed from
  // the cache when the threads complete

  // run the test multiple times to create many thread contexts for the same thread context collection
  Config cfg;
  ThreadContextCollection thread_context_collection{cfg};

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
      threads[i] = std::thread([&thread_terminate_flag, &threads_started, &thread_context_collection]()
        {
          // create a context for that thread
          QUILL_MAYBE_UNUSED auto tc = thread_context_collection.local_thread_context<QUILL_QUEUE_TYPE>();
          threads_started.fetch_add(1);
          while (!thread_terminate_flag.load())
          {
            // loop waiting for main to signal
            std::this_thread::sleep_for(std::chrono::nanoseconds{10});
          }
        });
    }

    // main wait for all of them to start
    while (threads_started.load() < num_threads)
    {
      // loop until all threads start
      // Instead of just waitng we will just call get cached contexts many times for additional testing
      thread_context_collection.clear_invalid_and_empty_thread_contexts();
      QUILL_MAYBE_UNUSED auto& cached_thread_contexts =
        thread_context_collection.backend_thread_contexts_cache();
    }

    // Check we have exactly as many thread contexts as the amount of threads in our backend cache
    thread_context_collection.clear_invalid_and_empty_thread_contexts();
    REQUIRE_EQ(thread_context_collection.backend_thread_contexts_cache().size(), num_threads);

    // Check all thread contexts in the backend thread contexts cache
    thread_context_collection.clear_invalid_and_empty_thread_contexts();
    for (auto& thread_context : thread_context_collection.backend_thread_contexts_cache())
    {
      REQUIRE(thread_context->is_valid());
      REQUIRE(thread_context->spsc_queue<QUILL_QUEUE_TYPE>().empty());
      thread_context_collection.clear_invalid_and_empty_thread_contexts();
    }

    // terminate all threads - This will invalidate all the contracts
    for (size_t j = 0; j < threads.size(); ++j)
    {
      terminate_flag[j].store(true);
      threads[j].join();
    }

    // keep loading the cache until it is empty - it will be empty when all threads have joined
    thread_context_collection.clear_invalid_and_empty_thread_contexts();
    while (!thread_context_collection.backend_thread_contexts_cache().empty())
    {
      // The cache is not empty yet it means that we still have joinable threads so wait for them to finish
      auto found_joinable = std::any_of(threads.begin(), threads.end(),
                                        [](std::thread const& th) { return th.joinable(); });

      REQUIRE(found_joinable);
      thread_context_collection.clear_invalid_and_empty_thread_contexts();
    }

    // Check there is no thread context left by getting the updated cache via the call
    // to backend_thread_contexts_cache()
    thread_context_collection.clear_invalid_and_empty_thread_contexts();
    REQUIRE_EQ(thread_context_collection.backend_thread_contexts_cache().size(), 0);

    // Finally call join on everything
    for (size_t j = 0; j < threads.size(); ++j)
    {
      terminate_flag[j].store(true);
    }
  }
}

TEST_SUITE_END();