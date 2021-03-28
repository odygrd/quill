#include "doctest/doctest.h"

#include "quill/detail/misc/Spinlock.h"
#include <array>
#include <mutex>
#include <random>
#include <thread>

TEST_SUITE_BEGIN("Spinlock");

namespace
{
struct LockedVal
{
  std::array<uint32_t, 1024> ar;
  quill::detail::Spinlock lock;

  LockedVal() { ar.fill(0); }
};

void spinlock_test_thread(LockedVal* v)
{
  constexpr size_t max = 1000;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint32_t> dis(1, (std::numeric_limits<uint32_t>::max)());

  for (int i = 0; i < max; i++)
  {
    std::lock_guard<quill::detail::Spinlock> const lock(v->lock);

    // Get the first value and check all other values are the same
    uint32_t const first = v->ar[0];
    for (auto const elem : v->ar)
    {
      // Do not use REQUIRE because in doctest it is thread safe and makes the test very slow
      if (first != elem)
      {
          FAIL("first should not be equal to elem. first: " << first << " elem: " << elem);
      }
    }

    // Set whole array to new value ( will be checked by the next thread )
    v->ar.fill(dis(gen));
  }
}

struct TryLockState
{
  quill::detail::Spinlock lock1;
  quill::detail::Spinlock lock2;
  bool locked{false};
  uint64_t obtained{0};
  uint64_t failed{0};
};

void try_lock_test_thread(TryLockState* state, size_t count)
{
  while (true)
  {
    std::lock_guard<quill::detail::Spinlock> const lock(state->lock1);

    if (state->obtained >= count)
    {
      break;
    }

    bool const ret = state->lock2.try_lock();
    // Do not use REQUIRE because in doctest it is thread safe and makes the test very slow
    if (state->locked == ret)
    {
      FAIL("state->locked should not be equal to ret. state->locked: " << state->locked << " ret: " << ret);
    }

    if (ret)
    {
      // We got lock2.
      ++state->obtained;
      state->locked = true;

      // Release lock1 and wait until at least one other thread fails to
      // obtain the lock2 before continuing.
      auto old_failed = state->failed;
      while (state->failed == old_failed && state->obtained < count)
      {
        state->lock1.unlock();
        state->lock1.lock();
      }

      state->locked = false;
      state->lock2.unlock();
    }
    else
    {
      ++state->failed;
    }
  }
}

} // namespace

TEST_CASE("lock")
{
  constexpr size_t num_threads = 20;

  std::vector<std::thread> threads;
  threads.reserve(num_threads);

  LockedVal spinlock;

  for (int i = 0; i < num_threads; ++i)
  {
    threads.emplace_back(std::thread(spinlock_test_thread, &spinlock));
  }

  for (auto& t : threads)
  {
    t.join();
  }
}

TEST_CASE("try_lock")
{
  constexpr size_t num_threads = 20;

  std::vector<std::thread> threads;
  threads.reserve(num_threads);

  TryLockState state;
  size_t count = 100;

  for (int i = 0; i < num_threads; ++i)
  {
    threads.emplace_back(std::thread(try_lock_test_thread, &state, count));
  }

  for (auto& t : threads)
  {
    t.join();
  }

  REQUIRE_EQ(count, state.obtained);

  // Each time the code obtains lock2 it waits for another thread to fail
  // to acquire it.
  // The only time this might not happen is on the very last loop when no other threads are left.
  REQUIRE_GE(state.failed + 1, state.obtained);
}

TEST_SUITE_END();