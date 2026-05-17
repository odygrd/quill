#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/BackendTscClock.h"

using namespace quill;

/***/
TEST_CASE("backend_tsc_clock_invalid_config_throws")
{
  BackendOptions backend_options;
  backend_options.sleep_duration = std::chrono::milliseconds{10};
  backend_options.rdtsc_resync_interval = std::chrono::milliseconds{1};

  Backend::start(backend_options);

  BackendTscClock::RdtscVal const tsc = BackendTscClock::rdtsc();

  REQUIRE_THROWS_AS([&]() { (void)Backend::convert_rdtsc_to_epoch_time(tsc.value()); }(), QuillError);
  REQUIRE_THROWS_AS([&]() { (void)BackendTscClock::now(); }(), QuillError);
  REQUIRE_THROWS_AS([&]() { (void)BackendTscClock::to_time_point(tsc); }(), QuillError);

  Backend::stop();
}
