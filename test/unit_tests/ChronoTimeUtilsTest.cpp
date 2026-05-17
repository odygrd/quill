#include "doctest/doctest.h"

#include "quill/core/ChronoTimeUtils.h"

#include <chrono>
#include <cstdint>
#include <thread>

TEST_SUITE_BEGIN("ChronoTimeUtils");

using namespace quill;

TEST_CASE("get_system_time_ns_matches_chrono_system_clock")
{
  // Allow a generous tolerance for scheduler jitter between the two calls.
  // Both clocks measure wall time so they should agree closely; 100 ms is
  // plenty to absorb CI noise without masking real bugs (a broken epoch
  // offset or unit scale would be off by years or orders of magnitude).
  constexpr int64_t tolerance_ns = 100'000'000;

  for (int i = 0; i < 5; ++i)
  {
    auto const chrono_before = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                 std::chrono::system_clock::now().time_since_epoch())
                                 .count();
    uint64_t const ours = detail::get_system_time_ns();
    auto const chrono_after = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                std::chrono::system_clock::now().time_since_epoch())
                                .count();

    // Sanity: the std::chrono readings must be monotonic here by construction.
    REQUIRE_LE(chrono_before, chrono_after);

    auto const ours_signed = static_cast<int64_t>(ours);

    // ours should lie inside [chrono_before - tolerance, chrono_after + tolerance].
    REQUIRE_GE(ours_signed, chrono_before - tolerance_ns);
    REQUIRE_LE(ours_signed, chrono_after + tolerance_ns);
  }
}

TEST_CASE("get_steady_time_ns_matches_chrono_steady_clock")
{
  constexpr int64_t tolerance_ns = 100'000'000;

  for (int i = 0; i < 5; ++i)
  {
    auto const chrono_before = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                 std::chrono::steady_clock::now().time_since_epoch())
                                 .count();
    uint64_t const ours = detail::get_steady_time_ns();
    auto const chrono_after = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                std::chrono::steady_clock::now().time_since_epoch())
                                .count();

    REQUIRE_LE(chrono_before, chrono_after);

    auto const ours_signed = static_cast<int64_t>(ours);

    REQUIRE_GE(ours_signed, chrono_before - tolerance_ns);
    REQUIRE_LE(ours_signed, chrono_after + tolerance_ns);
  }
}

TEST_CASE("get_steady_time_ns_is_monotonic")
{
  constexpr int iterations = 1000;

  uint64_t prev = detail::get_steady_time_ns();
  for (int i = 0; i < iterations; ++i)
  {
    uint64_t const now = detail::get_steady_time_ns();
    REQUIRE_GE(now, prev);
    prev = now;
  }
}

TEST_CASE("get_steady_time_ns_elapsed_matches_chrono")
{
  constexpr auto sleep_duration = std::chrono::milliseconds{50};

  uint64_t const ours_begin = detail::get_steady_time_ns();
  auto const chrono_begin = std::chrono::steady_clock::now();

  std::this_thread::sleep_for(sleep_duration);

  uint64_t const ours_end = detail::get_steady_time_ns();
  auto const chrono_end = std::chrono::steady_clock::now();

  auto const ours_elapsed_ns = static_cast<int64_t>(ours_end - ours_begin);
  auto const chrono_elapsed_ns =
    std::chrono::duration_cast<std::chrono::nanoseconds>(chrono_end - chrono_begin).count();

  // Both must have observed at least the requested sleep.
  auto const sleep_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(sleep_duration).count();
  REQUIRE_GE(ours_elapsed_ns, sleep_ns);
  REQUIRE_GE(chrono_elapsed_ns, sleep_ns);

  // The two measurements should agree to within 10 ms — large enough for
  // scheduler noise, small enough to catch any unit-scale bug.
  constexpr int64_t agreement_tolerance_ns = 10'000'000;
  auto const diff = ours_elapsed_ns > chrono_elapsed_ns ? (ours_elapsed_ns - chrono_elapsed_ns)
                                                        : (chrono_elapsed_ns - ours_elapsed_ns);
  REQUIRE_LT(diff, agreement_tolerance_ns);
}

TEST_CASE("get_system_time_ns_is_post_2020")
{
  // 2020-01-01 00:00:00 UTC in ns since Unix epoch.
  constexpr uint64_t jan_2020_ns = 1'577'836'800ull * 1'000'000'000ull;
  // 2100-01-01 00:00:00 UTC in ns since Unix epoch — sanity upper bound.
  constexpr uint64_t jan_2100_ns = 4'102'444'800ull * 1'000'000'000ull;

  uint64_t const ts = detail::get_system_time_ns();
  REQUIRE_GT(ts, jan_2020_ns);
  REQUIRE_LT(ts, jan_2100_ns);
}

TEST_SUITE_END();
