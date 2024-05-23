#include "doctest/doctest.h"

#include "quill/backend/RdtscClock.h"
#include "quill/core/Rdtsc.h"
#include <chrono>
#include <thread>

TEST_SUITE_BEGIN("RdtscClock");

void check_wall_time_now(quill::detail::RdtscClock const& tsc_clock, size_t& failures)
{
  std::chrono::milliseconds constexpr offset{10};

  auto const wall_time_chrono = std::chrono::system_clock::now().time_since_epoch();
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

TEST_SUITE_END();