#include "quill/backend/RdtscClock.h"
#include "doctest/doctest.h"

#include "quill/core/Rdtsc.h"

#include <atomic>
#include <chrono>
#include <thread>

TEST_SUITE_BEGIN("RdtscClock");

void check_wall_time_now(quill::detail::RdtscClock const& tsc_clock, size_t& failures)
{
  std::chrono::milliseconds constexpr offset{10};

  auto const wall_time_chrono = std::chrono::nanoseconds{quill::detail::get_system_time_ns()};
  auto const wall_time_tsc = std::chrono::nanoseconds{tsc_clock.time_since_epoch(quill::detail::rdtsc())};

  auto const lower_bound = wall_time_chrono - offset;
  auto const upper_bound = wall_time_chrono + offset;

  if (!((wall_time_tsc > lower_bound) && (wall_time_tsc < upper_bound)))
  {
    ++failures;

    if (failures > 1)
    {
      // wall_time_tsc is not between wall_time_chrono - 1 and wall_time_chrono + 1
      FAIL("wall_time_tsc: " << wall_time_tsc.count() << " lower_bound: " << lower_bound.count()
                             << " upper_bound: " << upper_bound.count() << "\n");
    }
  }
}

TEST_CASE("wall_time")
{
  quill::detail::RdtscClock const tsc_clock{std::chrono::milliseconds{1200}};

  constexpr size_t num_reps{10};
  size_t failures{0};

  for (size_t i = 1; i <= num_reps; ++i)
  {
    check_wall_time_now(tsc_clock, failures);
    std::this_thread::sleep_for(std::chrono::milliseconds{i * 100});
  }

  REQUIRE_LE(failures, 1);
}

class RdtscClockMock : public quill::detail::RdtscClock
{
public:
  using quill::detail::RdtscClock::RdtscClock;

  int64_t& resync_interval_ticks() { return _resync_interval_ticks; }
  int64_t& resync_interval_original() { return _resync_interval_original; }
  double& ns_per_tick() { return _ns_per_tick; }
  std::atomic<uint32_t>& version() { return _version; }
  std::array<BaseTimeTsc, 2>& base() { return _base; }

  void publish_snapshot(int64_t base_time, uint64_t base_tsc)
  {
    auto const index = (_version.load(std::memory_order_relaxed) + 1u) & (_base.size() - 1u);
    _base[index].base_time = base_time;
    _base[index].base_tsc = base_tsc;
    _version.fetch_add(1u, std::memory_order_release);
  }
};

TEST_CASE("time_since_epoch_safe_handles_concurrent_slot_reuse")
{
  RdtscClockMock tsc_clock{std::chrono::milliseconds{1200}};
  tsc_clock.ns_per_tick() = 1.0;

  constexpr uint64_t rdtsc_value{1'000'000};
  constexpr uint64_t expected_wall_time{10'000'000};
  constexpr uint32_t iterations{100'000};

  tsc_clock.publish_snapshot(static_cast<int64_t>(expected_wall_time), rdtsc_value);

  std::atomic<bool> start{false};
  std::atomic<uint32_t> ready{0};
  std::atomic<uint32_t> mismatches{0};

  auto reader = [&]()
  {
    ready.fetch_add(1u, std::memory_order_release);
    while (!start.load(std::memory_order_acquire))
    {
      std::this_thread::yield();
    }

    for (uint32_t i = 0; i < iterations; ++i)
    {
      if (tsc_clock.time_since_epoch_safe(rdtsc_value) != expected_wall_time)
      {
        mismatches.fetch_add(1u, std::memory_order_relaxed);
      }
    }
  };

  std::thread reader_one{reader};
  std::thread reader_two{reader};

  while (ready.load(std::memory_order_acquire) != 2u)
  {
    std::this_thread::yield();
  }
  start.store(true, std::memory_order_release);

  for (uint32_t generation = 1; generation <= iterations; ++generation)
  {
    tsc_clock.publish_snapshot(static_cast<int64_t>(expected_wall_time + generation), rdtsc_value + generation);
    if ((generation & 0xffu) == 0u)
    {
      std::this_thread::yield();
    }
  }

  reader_one.join();
  reader_two.join();

  REQUIRE_EQ(mismatches.load(std::memory_order_relaxed), 0u);
}

TEST_CASE("time_since_epoch_uses_resynced_slot")
{
  RdtscClockMock tsc_clock{std::chrono::milliseconds{1200}};

  // Force the next call to trigger a resync so we can verify which slot is used afterwards.
  tsc_clock.resync_interval_ticks() = 0;
  tsc_clock.resync_interval_original() = 0;

  bool observed_resync = false;

  for (size_t attempt = 0; attempt < 16; ++attempt)
  {
    uint32_t const version_before = tsc_clock.version().load(std::memory_order_relaxed);
    size_t const old_index = version_before & (tsc_clock.base().size() - 1);

    uint64_t const rdtsc_value = quill::detail::rdtsc();

    // Make the currently active slot a stale sentinel before forcing resync. If time_since_epoch()
    // accidentally keeps using this old slot after resync, the result will be near 42 instead of
    // the real wall-clock timestamp from the new slot.
    constexpr int64_t stale_base_time = 42;
    tsc_clock.base()[old_index].base_time = stale_base_time;
    tsc_clock.base()[old_index].base_tsc = rdtsc_value - 1;

    uint64_t const result = tsc_clock.time_since_epoch(rdtsc_value);

    uint32_t const version_after = tsc_clock.version().load(std::memory_order_relaxed);
    if (version_after == version_before)
    {
      continue;
    }

    observed_resync = true;

    size_t const new_index = version_after & (tsc_clock.base().size() - 1);
    REQUIRE_NE(new_index, old_index);

    auto const diff_new = static_cast<int64_t>(rdtsc_value - tsc_clock.base()[new_index].base_tsc);
    uint64_t const expected_new =
      static_cast<uint64_t>(tsc_clock.base()[new_index].base_time +
                            static_cast<int64_t>(static_cast<double>(diff_new) * tsc_clock.ns_per_tick()));

    auto const diff_stale = static_cast<int64_t>(rdtsc_value - tsc_clock.base()[old_index].base_tsc);
    uint64_t const expected_from_stale_slot = static_cast<uint64_t>(
      tsc_clock.base()[old_index].base_time +
      static_cast<int64_t>(static_cast<double>(diff_stale) * tsc_clock.ns_per_tick()));

    REQUIRE_EQ(result, expected_new);
    REQUIRE_NE(result, expected_from_stale_slot);
    break;
  }

  REQUIRE(observed_resync);
}

TEST_SUITE_END();
