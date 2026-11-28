// The headers of a file that logs come first, so that the check below sees only what they include
#include "quill/LogMacros.h"
#include "quill/Logger.h"

// The logging macros do not include <chrono> (docs/logging_macros.rst). With the MSVC standard
// library, whose C++20 <chrono> also brings in <format> and the time zone headers,
// core/ChronoTimeUtils.h reads the clocks without it
#if defined(_MSVC_STL_VERSION) && defined(_CHRONO_)
  #error "quill/Logger.h and quill/LogMacros.h must not include <chrono>"
#endif

#include "doctest/doctest.h"

#include <cstdint>

TEST_SUITE_BEGIN("ThreadPrimitives");

using namespace quill;

TEST_CASE("sleep_for_ns_waits_at_least_a_duration_below_one_millisecond")
{
  // On Windows a wait below 1 ms yields until it has passed instead of sleeping a timer tick
  constexpr uint64_t duration_ns{200'000};

  uint64_t const start_ns = detail::get_steady_time_ns();
  detail::sleep_for_ns(duration_ns);
  uint64_t const elapsed_ns = detail::get_steady_time_ns() - start_ns;

  REQUIRE_GE(elapsed_ns, duration_ns);
}

TEST_SUITE_END();
