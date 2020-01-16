#include "quill/detail/ThreadContextCollection.h"
#include <array>
#include <gtest/gtest.h>
#include <thread>

using namespace quill::detail;

/***/
TEST(ThreadContextCollection, add_remove_thread_context)
{
  ThreadContextCollection thread_context_collection;

  // Try to get the context of the same thread many times and check nothing has changed
  constexpr uint32_t tries = 3;
  for (int i = 0; i < tries; ++i)
  {
    // Get this thread context
    auto* thread_context = thread_context_collection.get_local_thread_context();

    EXPECT_EQ(thread_context->is_valid(), true);
    EXPECT_EQ(thread_context->spsc_queue().empty(), true);

    // Get all thread contexts from the thread context collection
    std::vector<ThreadContext*> const& cached_thread_contexts =
      thread_context_collection.get_cached_thread_contexts();

    EXPECT_EQ(cached_thread_contexts.size(), 1);
  }

  // invalidate the context
  auto* thread_context = thread_context_collection.get_local_thread_context();
  thread_context->invalidate();
  EXPECT_EQ(thread_context->is_valid(), false);

  // remove it
  thread_context_collection.remove_thread_context(thread_context);

  // Get all thread contexts from the thread context collection
  std::vector<ThreadContext*> const& cached_thread_contexts =
    thread_context_collection.get_cached_thread_contexts();

  EXPECT_TRUE(cached_thread_contexts.empty());
}

/***/
TEST(ThreadContextCollection, add_remove_thread_context_multithreaded)
{
  // run the test multiple times to create many thread contexts
  ThreadContextCollection thread_context_collection;

  constexpr uint32_t tries = 3;
  for (int k = 0; k < tries; ++k)
  {
    constexpr size_t num_threads{20};
    std::array<std::thread, num_threads> threads;
    std::array<std::atomic<bool>, num_threads> terminate_flag{false};
    std::atomic<uint32_t> threads_started{0};

    // spawn x number of threads
    for (size_t i = 0; i < threads.size(); ++i)
    {
      auto& thread_terminate_flag = terminate_flag[i];
      threads[i] = std::thread([&thread_terminate_flag, &threads_started, &thread_context_collection]() {
        // create a context for that thread
        [[maybe_unused]] auto tc = thread_context_collection.get_local_thread_context();
        threads_started.fetch_add(1);
        while (!thread_terminate_flag.load())
        {
          // loop waiting for main to signal
        }
      });
    }

    // main wait for all of them to start
    while (threads_started.load() < num_threads)
    {
      // loop until all threads start but all call get cached contexts many times
      [[maybe_unused]] auto& cached_thread_contexts = thread_context_collection.get_cached_thread_contexts();
    }

    // Check we have exactly as many thread contexts as the amount of threads
    EXPECT_EQ(thread_context_collection.get_cached_thread_contexts().size(), num_threads);

    // Check all thread contexts
    for (auto& thread_context : thread_context_collection.get_cached_thread_contexts())
    {
      EXPECT_TRUE(thread_context->is_valid());
      EXPECT_TRUE(thread_context->spsc_queue().empty());
    }

    // terminate all threds
    for (size_t j = 0; j < threads.size(); ++j)
    {
      terminate_flag[j].store(true);
      threads[j].join();
    }

    // Now check all thread contexts still exist but they are invalided and then remove them
    for (auto* thread_context : thread_context_collection.get_cached_thread_contexts())
    {
      EXPECT_FALSE(thread_context->is_valid());
      EXPECT_TRUE(thread_context->spsc_queue().empty());

      thread_context_collection.remove_thread_context(thread_context);
    }

    // Check there is no thread context left
    EXPECT_EQ(thread_context_collection.get_cached_thread_contexts().size(), 0);
  }
}