#include "doctest/doctest.h"

#include "quill/StopWatch.h"
#include <thread>

TEST_SUITE_BEGIN("StopWatch");

TEST_CASE("stopwatch_tsc")
{
  quill::StopWatchTsc swt;
  std::this_thread::sleep_for(std::chrono::seconds{1});

  // greater than 1 second
  REQUIRE_GE(swt.elapsed().count(), 1.0);
  REQUIRE_GE(swt.elapsed_as<std::chrono::nanoseconds>().count(), 1'000'000'000);
  REQUIRE_GE(swt.elapsed_as<std::chrono::microseconds>().count(), 1'000'000);
  REQUIRE_GE(swt.elapsed_as<std::chrono::milliseconds>().count(), 1'000);
  REQUIRE_GE(swt.elapsed_as<std::chrono::seconds>().count(), 1);

  swt.reset();
  REQUIRE_LT(swt.elapsed().count(), 1.0);

  std::this_thread::sleep_for(std::chrono::seconds{2});

  // greater than 2 seconds
  REQUIRE_GE(swt.elapsed().count(), 2.0);
  REQUIRE_GE(swt.elapsed_as<std::chrono::nanoseconds>().count(), 2'000'000'000);
  REQUIRE_GE(swt.elapsed_as<std::chrono::microseconds>().count(), 2'000'000);
  REQUIRE_GE(swt.elapsed_as<std::chrono::milliseconds>().count(), 2'000);
  REQUIRE_GE(swt.elapsed_as<std::chrono::seconds>().count(), 2);
}

TEST_CASE("stopwatch_chrono")
{
  quill::StopWatchChrono swt;
  std::this_thread::sleep_for(std::chrono::seconds{1});

  // greater than 1 second
  REQUIRE_GE(swt.elapsed().count(), 1.0);
  REQUIRE_GE(swt.elapsed_as<std::chrono::nanoseconds>().count(), 1'000'000'000);
  REQUIRE_GE(swt.elapsed_as<std::chrono::microseconds>().count(), 1'000'000);
  REQUIRE_GE(swt.elapsed_as<std::chrono::milliseconds>().count(), 1'000);
  REQUIRE_GE(swt.elapsed_as<std::chrono::seconds>().count(), 1);

  swt.reset();
  REQUIRE_LT(swt.elapsed().count(), 1.0);

  std::this_thread::sleep_for(std::chrono::seconds{2});

  // greater than 2 seconds
  REQUIRE_GE(swt.elapsed().count(), 2.0);
  REQUIRE_GE(swt.elapsed_as<std::chrono::nanoseconds>().count(), 2'000'000'000);
  REQUIRE_GE(swt.elapsed_as<std::chrono::microseconds>().count(), 2'000'000);
  REQUIRE_GE(swt.elapsed_as<std::chrono::milliseconds>().count(), 2'000);
  REQUIRE_GE(swt.elapsed_as<std::chrono::seconds>().count(), 2);
}

TEST_SUITE_END();